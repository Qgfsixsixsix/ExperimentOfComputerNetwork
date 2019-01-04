/**
* ////////////////// file textfour.cpp
*
* author : qgfzzzzzz
* date : 2019 / 01 / 03
* version : 1.0
* title : Network Sniffer
*/

/**
* 本程序作用在本主机上数据链路程
* 对以太网设备上传送的数据包进行监听，也可以说是在本网段以太网上（数据链路层）传输的数据包
*/
#include <stdio.h>
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <stdlib.h>
#include <Windows.h>
#include <string.h>

#define MAX_ADDR_LEN 32 //ip地址的长度

/**
* 网卡有多种工作模式
* 	广播模式 (broadcast)
* 	多播模式 (multicast)
* 	单播模式 (unicast)
* 	混杂模式 (Promiscuous)
*/
#define SIO_RCVALL  (IOC_IN|IOC_VENDOR|1)//定义网卡为混杂模式，这样就可以监听以太网上传输的每个数据包

typedef struct ip_hdr//定义IP首部
{
    unsigned char h_verlen;//4位首部长度，4位IP版本号
    unsigned char tos;//8位服务类型TOS
    unsigned short tatal_len;//16位总长度
    unsigned short ident;//16位标示
    unsigned short frag_and_flags;//偏移量和3位标志位
    unsigned char ttl;//8位生存时间TTL
    unsigned char proto;//8位协议（TCP,UDP或其他）
    unsigned short checksum;//16位IP首部检验和
    unsigned int sourceIP;//32位源IP地址
    unsigned int destIP;//32位目的IP地址
} IPHEADER;

typedef struct tsd_hdr//定义TCP伪首部
{
    unsigned long saddr;//源地址
    unsigned long daddr;//目的地址
    char mbz;
    char ptcl;//协议类型
    unsigned short tcpl;//TCP长度
} PSDHEADER;

/**
* UDP用户数据报首部中检验和的计算方法有点特殊，需要在UDP数据报前面添加12个字节的伪首部
* 伪首部就是只在计算检验和时，临时添加在UDP用户数据前面，得到一个临时的UDP用户数据报
* 校验和就是按照这个临时的UDP数据报来计算的，既不向下传也不向上传，仅仅时为了计算检验和
*/
typedef struct tcp_hdr//定义TCP首部
{
    unsigned short sport;//16位源端口
    unsigned short dport;//16位目的端口
    unsigned int seq;//32位序列号
    unsigned int ack;//32位确认号
    unsigned char lenres;//4位首部长度/6位保留字
    unsigned char flag;//6位标志位
    unsigned short win;//16位窗口大小
    unsigned short sum;//16位检验和
    unsigned short urp;//16位紧急数据偏移量
} TCPHEADER;

typedef struct udp_hdr//定义UDP首部
{
    unsigned short sport;//16位源端口
    unsigned short dport;//16位目的端口
    unsigned short len;//UDP 长度
    unsigned short cksum;//检查和
} UDPHEADER;

typedef struct icmp_hdr//定义ICMP首部
{
    unsigned short sport;
    unsigned short dport;
    unsigned char type;
    unsigned char code;
    unsigned short cksum;
    unsigned short id;
    unsigned short seq;
    unsigned long timestamp;
} ICMPHEADER;

int main(int argc, char **argv)
{
    SOCKET sock;
    WSADATA wsd;
    char recvBuf[65535] = { 0 };
    char temp[65535] = { 0 };
    DWORD dwBytesRet;

    int pCount = 0;
    unsigned int optval = 1;
    unsigned char* dataip = NULL;
    unsigned char* datatcp = NULL;
    unsigned char* dataudp = NULL;
    unsigned char* dataicmp = NULL;

    int lentcp, lenudp, lenicmp, lenip;
    char TcpFlag[6] = { 'F', 'S', 'R','P', 'A', 'U' };//定义TCP标志位
    WSAStartup(MAKEWORD(2, 1), &wsd);//表示使用winsock2.1版本
    //创建一个原始套接字
    /**
	* 流式套接字(SOCK_STREAM) : 面向连接的套接字，对应于TCP应用程序
	* 数据报套接字(SOCK_DGRAM) : 无连接的套接字，对应于UDP应用程序
	* 以上就是标准的套接字
	*
	* 原始套接字(SOCK_RAM) : 它是一种对原始网络报文进行处理的套接字,抓取数据报的信息肯定用原始套接字
	*/
    if ((sock = socket(AF_INET, SOCK_RAW, IPPROTO_IP)) == SOCKET_ERROR)//创建一个原始套接字
    {
        printf("create socket error!\n");
        exit(0);
    }

    char FAR name[MAXBYTE];
    gethostname(name, MAXBYTE);//把本地主机名存放入name数组中

    struct hostent FAR* pHostent;

    pHostent = (struct hostent*)malloc(sizeof(struct hostent));
    pHostent = gethostbyname(name);//返回对应于给定主机的包含主机名字和地址信息的hostent指针
    SOCKADDR_IN sa;
    sa.sin_family = AF_INET;
    sa.sin_port = htons(1);//原始套接字没有端口的概念，所以这个值随便设置
    memcpy(&sa.sin_addr,pHostent->h_addr_list[0],pHostent->h_length);//设置本机地址
    bind(sock, (SOCKADDR*)&sa, sizeof(sa));//绑定

    if (WSAGetLastError() == 10013)
    {
        exit(0);
    }

    //设置网卡为混杂模式，也叫泛听模式。可以侦听经过的所有的包。
    WSAIoctl(sock, SIO_RCVALL, &optval, sizeof(optval), nullptr, 0, &dwBytesRet, nullptr, nullptr);

    UDPHEADER * pUdpheader;//UDP头结构体指针
    IPHEADER * pIpheader;//IP头结构体指针
    TCPHEADER * pTcpheader;//TCP头结构体指针
    ICMPHEADER * pIcmpheader;//ICMP头结构体指针
    char szSourceIP[MAX_ADDR_LEN], szDestIP[MAX_ADDR_LEN];//源IP和目的IP
    SOCKADDR_IN saSource, saDest;//源地址结构体，目的地址结构体

    //设置各种头指针
    pIpheader = (IPHEADER*)recvBuf;
    pTcpheader = (TCPHEADER*)(recvBuf + sizeof(IPHEADER));
    pUdpheader = (UDPHEADER*)(recvBuf + sizeof(IPHEADER));
    pIcmpheader = (ICMPHEADER*)(recvBuf + sizeof(IPHEADER));
    //int iIphLen = sizeof(unsigned long)*(pIpheader->h_verlen & 0x0f);
    while (1)
    {

        memset(recvBuf, 0, sizeof(recvBuf));//清空缓冲区
        recv(sock, recvBuf, sizeof(recvBuf), 0);//接收包

        //获得源地址和目的地址
        saSource.sin_addr.s_addr = pIpheader->sourceIP;
        //将源地址复制到szSourceIP数组中
        strncpy(szSourceIP, inet_ntoa(saSource.sin_addr), MAX_ADDR_LEN);
        saDest.sin_addr.s_addr = pIpheader -> destIP;
        //将目的地址复制到szDestIP数组中
        strncpy(szDestIP, inet_ntoa(saDest.sin_addr), MAX_ADDR_LEN);

        //计算各种包的长度（只有判断是否是该包后才有意义，先计算出来）
        lenip = ntohs(pIpheader -> tatal_len);
        lentcp = ntohs(pIpheader -> tatal_len) - (sizeof(IPHEADER) + sizeof(TCPHEADER));
        lenudp = ntohs(pIpheader -> tatal_len) - (sizeof(IPHEADER) + sizeof(UDPHEADER));
        lenicmp = ntohs(pIpheader -> tatal_len) - (sizeof(IPHEADER) + sizeof(ICMPHEADER));

        //判断是否是TCP包
        if (pIpheader->proto == IPPROTO_TCP&&lentcp != 0)
        {
            pCount++;//计数加一
            dataip = (unsigned char *)recvBuf;
            datatcp = (unsigned char *)recvBuf + sizeof(IPHEADER) + sizeof(TCPHEADER);
            system("cls");
            printf("接收到第 %d 的数据包，含有%d字节数据\n",pCount,lentcp);
            printf("---------- IP协议头部 -----------\n");
            printf("标示：%i\n", ntohs(pIpheader->ident));
            printf("总长度：%i\n", ntohs(pIpheader->tatal_len));
            printf("偏移量：%i\n", ntohs(pIpheader->frag_and_flags));
            printf("生存时间：%d\n",pIpheader->ttl);
            printf("服务类型：%d\n",pIpheader->tos);
            printf("协议类型：%d\n",pIpheader->proto);
            printf("检验和：%i\n", ntohs(pIpheader->checksum));
            printf("源IP：%s\n", szSourceIP);
            printf("目的IP：%s\n", szDestIP);
            printf("---------- TCP协议头部 ----------\n");
            printf("源端口：%i\n", ntohs(pTcpheader->sport));
            printf("目的端口：%i\n", ntohs(pTcpheader->dport));
            printf("序列号：%i\n", ntohs(pTcpheader->seq));
            printf("应答号：%i\n", ntohs(pTcpheader->ack));
            printf("检验和：%i\n", ntohs(pTcpheader->sum));
            printf("标志位：");

            unsigned char FlagMask = 1;
            int k;

            //打印标志位
            /*for (k = 0; k < 6; k++)
            {
                if ((pTcpheader->flag)&FlagMask)
                    printf("%c", TcpFlag[k]);
                else
                    printf(" ");
                FlagMask = FlagMask << 1;
            }*/
           /* printf("\n数据：\n");
            for (int i = 0; i < lentcp; i++)
            {
                printf("%x", datatcp[i]);
            }*/
            Sleep(500);
        }
        else if (pIpheader->proto == IPPROTO_UDP&&lenudp!= 0)
        {
            pCount++;//计数加一
            dataip = (unsigned char *)recvBuf;
            dataudp = (unsigned char *)recvBuf + sizeof(IPHEADER) + sizeof(UDPHEADER);
            system("cls");

            printf("接收到第 %d 的数据包，含有%d字节数据\n",pCount,lenudp);
            printf("---------- IP协议头部 ----------\n");
            printf("标示：%i\n", ntohs(pIpheader -> ident));
            printf("总长度：%i\n", ntohs(pIpheader -> tatal_len));
            printf("偏移量：%i\n", ntohs(pIpheader -> frag_and_flags));
            printf("生存时间：%d\n",pIpheader->ttl);
            printf("服务类型：%d\n",pIpheader->tos);
            printf("协议类型：%d\n",pIpheader->proto);
            printf("检验和：%i\n", ntohs(pIpheader->checksum));
            printf("源IP：%s\n", szSourceIP);
            printf("目的IP：%s\n", szDestIP);
            printf("----------- UDP协议头部 --------\n");
            printf("源端口：%i\n", ntohs(pUdpheader->sport));
            printf("目的端口：%i\n", ntohs(pUdpheader->dport));
            printf("Udp长度：%i\n", ntohs(pUdpheader->len));
            printf("检验和：%i\n", ntohs(pUdpheader->cksum));
            //printf("数据：\n");
            /*for (int i = 0; i < lenudp; i++)
            {
                printf("%x", dataudp[i]);
            }*/
            Sleep(500);
        }
    }
}
