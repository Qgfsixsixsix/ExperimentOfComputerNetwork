#include <stdio.h>
#include <iostream>
#include <Winsock2.h>
#include <windows.h>
#pragma comment(lib,"WSOCK32")
#define MaxSize 50
using namespace std;
HANDLE hMutex;
void Send(SOCKET sockClient){
    char sendBuf[MaxSize];
    int byte = 0;
    while(1){
        WaitForSingleObject(hMutex, INFINITE);
        gets(sendBuf);
        byte = send(sockClient, sendBuf, 50, 0);
        if(byte <= 0){
            break;
        }
        Sleep(1000);
        ReleaseMutex(hMutex);
    }
    closesocket(sockClient);
}

void Rec(SOCKET sockClient){
    char revBuf[MaxSize];
    int byte = 0;
    while(1){
        WaitForSingleObject(hMutex, INFINITE);
        byte = recv(sockClient, revBuf, 50, 0);
        if(byte <= 0){
            break;
        }
        printf("服务器 : %s\n", revBuf);
        Sleep(1000);
        ReleaseMutex(hMutex);//关闭socket

    }
    closesocket(sockClient);
}
int main()
{
    WORD wVersionRequested;
    WSADATA wsaData;
    int err;
    wVersionRequested = MAKEWORD( 1, 1 );
    err = WSAStartup( wVersionRequested, &wsaData );
    if ( err != 0 )
    {
        return -1;
    }
    if ( LOBYTE( wsaData.wVersion ) != 1 || HIBYTE( wsaData.wVersion ) != 1 )
    {
        WSACleanup( );
        return -1;
    }
    printf("################ 客户端 ################\n");
    printf("正在连接本机端口6000的服务器....\n");
    while(1){

        SOCKET sockClient=socket(AF_INET,SOCK_STREAM,0);
        SOCKADDR_IN addrSrv;
        addrSrv.sin_addr.S_un.S_addr=inet_addr("127.0.0.1");
        addrSrv.sin_family=AF_INET;
        addrSrv.sin_port=htons(6000);
        connect(sockClient,(SOCKADDR*)&addrSrv,sizeof(SOCKADDR));
        //send(sockClient,"hello",strlen("hello")+1,0);
        HANDLE hThread1 = CreateThread(NULL,0,(LPTHREAD_START_ROUTINE)Rec,(LPVOID)sockClient,0,0);
        if(hThread1 != NULL){
           CloseHandle(hThread1);
        }
        HANDLE hThread2 = CreateThread(NULL,0,(LPTHREAD_START_ROUTINE)Send,(LPVOID)sockClient,0,0);
        if(hThread2 != NULL){
            CloseHandle(hThread2);
        }
    }
//    char recvBuf[50];
//    recv(sockClient,recvBuf,50,0);
//    printf("%s\n",recvBuf);
//    closesocket(sockClient);
//    WSACleanup();
    getchar();
    WSACleanup();
    return 0;
}
