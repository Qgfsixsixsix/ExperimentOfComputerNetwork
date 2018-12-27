#include <stdio.h>
#include <Winsock2.h>
#include <windows.h>
#pragma comment(lib,"Ws2_32.lib")
#define MaxSize 50
using namespace std;
HANDLE hMutex;
//发送数据
void Send(SOCKET sockClient){
    char sendBuf[MaxSize];
    int byte = 0;
    while(1){
        //第一个参数：hHandle[in]对象句柄，可以指定一系列的对象，第二个参数，指定时间间隔完成线程间的通讯
        WaitForSingleObject(hMutex, INFINITE);
        gets(sendBuf);
        //服务器向客户端发送数据
        byte = send(sockClient, sendBuf, 50, 0);
        if(byte <= 0){
            break;
        }
        Sleep(1000);
        ReleaseMutex(hMutex);
    }
    closesocket(sockClient);
}

//接受数据
void Rec(SOCKET sockClient){
    char revBuf[MaxSize];
    int byte = 0;
    while(1){
        WaitForSingleObject(hMutex, INFINITE);
        //服务器从客户端接受数据
        byte = recv(sockClient, revBuf, 50, 0);
        if(byte <= 0){
            break;
        }
        printf("客户端 : %s\n", revBuf);
        Sleep(1000);
        ReleaseMutex(hMutex);
    }
    //关闭ocket, 一次通信完毕
    closesocket(sockClient);
}
int main()
{
    WORD wVersionRequested;
    WSADATA wsaData;
    int err;
    //这个wVersionRequested使我们要用到的版本
    wVersionRequested = MAKEWORD( 1, 1 );
    //初始化这个WinSock DLL库，调用这个接口函数可以初始化 WinSock
    err = WSAStartup(wVersionRequested, &wsaData);
    if ( err != 0 )
    {
        return -1;
    }
    /**
      *LOBYTE（）是取得16进制数最低（最右边）那个字节的内容
      *HIBYTE（）是取得16进制数最高（最左边）那个字节的内容
      *
     **/
    if ( LOBYTE( wsaData.wVersion ) != 1 || HIBYTE( wsaData.wVersion ) != 1 )
    {
        //WSACleanup是来解除与Socket库的绑定并且释放Socket库所占用的系统资源。
        WSACleanup();
        return -1;
    }
    //创建套接字，AF_INET是IP地址协议族，SOCK_STREAM是TCP/IP的流形式
    SOCKET sockSrv=socket(AF_INET,SOCK_STREAM,0);
    // 封装成sockaddr_in 类，俗称地址包，保存端口号和IP地址
    SOCKADDR_IN addrSrv;
    // 将IP地址 INADDR_ANY （0x00000）主机字节转换成网络字节
    addrSrv.sin_addr.S_un.S_addr=htonl(INADDR_ANY);
    // 设置为IP协议族
    addrSrv.sin_family=AF_INET;
    // 设置端口号，把主机字节转换成网络字节
    addrSrv.sin_port=htons(6000);
    //设置 IP 地址和 port 端口时，就必须把主机字节转化成网络字节后，
    //才能用 Bind()函数来绑定套接字和地址
    bind(sockSrv,(SOCKADDR*)&addrSrv,sizeof(SOCKADDR));
    //当绑定完成之后，服务器端必须建立一个监听的队列来接收客户端的连接请求
    //把套接字转换成监听模式
    listen(sockSrv,5);

    //这个类似于Java的封装，就是把地址和端口等信息封装到一块
    SOCKADDR_IN addrClient;
    int len=sizeof(SOCKADDR);
    //可以同时多个主机访问。类似于多线程
    printf("服务器正在开启....\n");
    printf("IP : 127.0.0.1 \n");
    printf("PORT : 6000\n");
    while(1)
    {
//        //接受客户端的请求
//        SOCKET sockConn=accept(sockSrv,(SOCKADDR*)&addrClient,&len);
//        //创建TCP传输的发送缓冲区，因为是全双工的，所以都可发送和接受
//        char sendBuf[50];
//        //inet_addr()返回的地址已经是网络字节格式
//        sprintf(sendBuf,"Welcome %s to here!",inet_ntoa(addrClient.sin_addr));
//        //进行发操作
//        send(sockConn,sendBuf,strlen(sendBuf)+1,0);
//        //创建TCP传输的接受缓冲区
//        char recvBuf[50];
//        //进行接操作
//        recv(sockConn,recvBuf,50,0inet_addr()返回的地址已经是网络字节格式);
//        printf("%s\n",recvBuf);
//        //关闭本次的连接，
//        closesocket(sockConn);

        //接受客户端的请求
        SOCKET sockClient = accept(sockSrv, (SOCKADDR*)&addrClient, &len);
        HANDLE hThread1 = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)Send, (LPVOID)sockClient, 0, 0);

        HANDLE hThread2 = CreateThread(NULL,0, (LPTHREAD_START_ROUTINE)Rec, (LPVOID)sockClient, 0, 0);
        if(hThread1 != NULL){
            CloseHandle(hThread1);
        }
        if(hThread2 != NULL){
            CloseHandle(hThread2);
        }
    }
    getchar();
    return 0;
}
