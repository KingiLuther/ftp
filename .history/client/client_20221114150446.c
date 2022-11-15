#include <stdio.h>
#include <stdbool.h>
#include <winsock2.h>
#pragma comment(lib, "ws2_32.lib")

//报错信息
#define err(errMsg) printf("[error]%s failed! code: %d line: %d", errMsg, WSAGetLastError(), __LINE__);
//端口信息
#define PORT 8888

/*与服务器连接,断开连接*/

bool init_Socket()
{
    WSADATA wsadata; 
    if(0 != WSAStartup(MAKEWORD(2, 2), &wsadata)){  //windows异步套接字启动失败
        err("WSAStartup()");
        return false;
    }
    return true;
}

bool close_Socket()
{
    if(0 != WSACleanup()){  //windows异步套接字关闭失败
        err("WSACleanup()");
        return false;
    }
    return true;
}

SOCKET create_clientSocket(const char* ip)
{
    //创建一个空的socket
    SOCKET fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);  //前两个参数我还没搞懂是什么意思，使用TCP协议
    if(INVALID_SOCKET == fd){   //创建失败
        err("socket()");
        return INVALID_SOCKET;
    }
    //给socket绑定服务端的IP地址和端口号
    struct sockaddr_in addr;                //包含端口号、IP地址等信息的结构体
    addr.sin_family = AF_INET;              //协议族需要与前面的协议族相同，不懂为什么
    addr.sin_port = htons(PORT);            //端口号8888，用htons把本地字节序8888转为网络字节序，不懂为什么
    addr.sin_addr.S_un.S_addr = inet_addr(ip);   //绑定本地任意IP地址
    //连接客户端服务器
    if(INVALID_SOCKET == connect(fd, (struct sockaddr*)&addr, sizeof(addr))){   //连接失败
        err("connect()");
        return INVALID_SOCKET;
    }
    return fd;
}

SOCKET connect_serverSocket()
{
    SOCKET serfd = create_clientSocket("127.0.0.1");
    printf("\n连接成功...\n");
    return serfd;
}

/*向服务器发送信息*/

bool send_Msg(SOCKET serfd, char* sendBuf)
{
    if(SOCKET_ERROR == send(serfd, sendBuf, strlen(sendBuf), 0)){ //发送失败
        err("send()");
        return false;
    }
    return true;
}





//客户端主函数
int main()
{
    //启动客户端
    init_Socket();
    SOCKET serfd = connect_serverSocket();


    //关闭一切
    closesocket(serfd);
    close_Socket();
    getchar();
    return 0;

}