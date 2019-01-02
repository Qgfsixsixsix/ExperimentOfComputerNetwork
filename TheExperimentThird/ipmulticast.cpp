#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <process.h>

//本程序使用的多播地址
#define MULTICAST_IP "233.0.0.1"

//绑定的本地端口
#define MULTICAST_PORT 5150

//接收数据缓冲区大小
#define MULTICAST_BUFFERSIZE 1024

using namespace std;

/**
* do_send()
* 发送函数
* void
*/
void do_send(void* arg){
	SOCKET server = (SOCKET)arg;
	char sendline[MULTICAST_BUFFERSIZE + 1];
	sockaddr_in remote;
	memset(&remote, 0, sizeof(remote)); //初始化

	remote.sin_addr.s_addr = inet_addr(MULTICAST_IP);
	remote.sin_family = AF_INET;
	remote.sin_port = htons(MULTICAST_PORT);

	//读取用户输入，直到用户输入 "end"
	while(1){
		cin.getline(sendline, MULTICAST_BUFFERSIZE);
		if(strncmp(sendline, "end", 3) == 0){
			break;
		}

		//发送用户输入的数据到多播组
		sendto(server, sendline, strlen(sendline), 0, (sockaddr*)(&remote), sizeof(remote));
	}
}

/**
* do_read()
* 接收函数
* void
*/
void do_read(void* arg){
	struct sockaddr_in from;
	SOCKET server = (SOCKET)arg;
	char buf[MULTICAST_BUFFERSIZE + 1];
	int ret;
	sockaddr_in client;
	int clientLen;

	//一直读取直到主线程终止
	while(1){
		clientLen = sizeof(client);
		memset(&client, 0, clientLen); //初始化
		ret = recvfrom(server, buf, MULTICAST_BUFFERSIZE, 0, (sockaddr*)&from, &clientLen);

		//do_read在用户直接回车发送一个空字符串
		if(ret == 0){
			continue;
		}
		else if(ret == SOCKET_ERROR){
			//主线程终止recvfrom返回的错
			if(WSAGetLastError() == WSAEINTR){
				break;
			}
			cout << "Error in recvfrom : " << WSAGetLastError() << endl;
			break;
		}
		buf[ret] = '\0';
		cout << "----------------------------IP : " << inet_ntoa(from.sin_addr) << ' ' << ntohs(from.sin_port)<< ' ' << buf << endl;
	}
}


int main(int args, char ** argv){
	WSADATA wsd;
	struct sockaddr_in local, remote;
	SOCKET sock;

	//初始化Winsock2.2
	if(WSAStartup(MAKEWORD(2,2), &wsd) != 0){
		cout << "WSAStartup() failed "<< endl;
		return -1;
	}

    /**
    * SOCK_DGRAM数据包套接字，分组是在不建立连接的情况下被发送到远程进程的
    * 采用UDP传输
    */
	if((sock = WSASocket(AF_INET, SOCK_DGRAM, 0, NULL, 0,
	WSA_FLAG_MULTIPOINT_C_LEAF | WSA_FLAG_MULTIPOINT_D_LEAF |
	WSA_FLAG_OVERLAPPED)) == INVALID_SOCKET){
		cout << "socket failed with : " << WSAGetLastError() << endl;
		WSACleanup();
		return -1;
	}

	//将sock 绑定到本地某端口上
	local.sin_family = AF_INET;
	local.sin_port = htons(MULTICAST_PORT);
	local.sin_addr.s_addr = INADDR_ANY; //网络字节
	if(bind(sock, (struct sockaddr*)&local, sizeof(local)) == SOCKET_ERROR){
		cout << "bind failed with : " << WSAGetLastError() << endl;
		closesocket(sock);
		WSACleanup();
		return -1;
	}

	//加入多播组
	remote.sin_family = AF_INET; //地址组协议（TCP/IP 协议）
	/**
	* 多播的传输与多播地址的端口没有关系
	* 多播地址的端口可以与本地的主机不一样，
	* 也就是说可以允许不同的端口的相同的多播组进行通信
	*/
	remote.sin_port = htons(MULTICAST_PORT); //绑定端口5151
	remote.sin_addr.s_addr = inet_addr(MULTICAST_IP); //转化为网络字节

	/**
	* 将一个节点加入一个多点会晤，交换连接数据，根据提供的流描述确认所需的服务
	* 返回值 若无错误发生，WSAJoinLeaf() 返回的是新创建的多点套接字的描述字， 否则的话，将返回的是INVALID_SOCKET错误
	*/
	if((WSAJoinLeaf(sock, (SOCKADDR*)&remote, sizeof(remote), NULL, NULL, NULL, NULL, JL_BOTH)) == INVALID_SOCKET) {
		cout << "WSAJoinLeaf() failed : " << WSAGetLastError() << endl;
		closesocket(sock);
		WSACleanup();
		return -1;
	}

	//创建两个线程，一个是读用户输入并发送
	HANDLE hHandle[2];

    //_beginthread()是对createThread的包装
	hHandle[0] = (HANDLE)_beginthread(do_send, 0, (void*)sock);
	hHandle[1] = (HANDLE)_beginthread(do_read, 0, (void*)sock);

	//如果用户输入结束，程序就终止
	WaitForSingleObject(hHandle[0], INFINITE);
	WSACleanup();
	return 0;
}
