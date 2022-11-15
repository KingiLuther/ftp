#include <stdio.h>
#include <stdbool.h>
#include <winsock2.h>
#pragma comment(lib, "ws2_32.lib")

//报错信息
#define err(errMsg) printf("[error]%s failed! code: %d line: %d", errMsg, WSAGetLastError(), __LINE__);
//端口信息
#define PORT 8888


//与客户端连接/断开连接

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

SOCKET create_serverSocket()
{
    //创建一个空的socket
    SOCKET fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);  //前两个参数我还没搞懂是什么意思，使用TCP协议
    if(INVALID_SOCKET == fd){   //创建失败
        err("socket()");
        return INVALID_SOCKET;
    }
    //给socket绑定本地IP地址和端口号
    struct sockaddr_in addr;                //包含端口号、IP地址等信息的结构体
    addr.sin_family = AF_INET;              //协议族需要与前面的协议族相同，不懂为什么
    addr.sin_port = htons(PORT);            //端口号8888，用htons把本地字节序8888转为网络字节序，不懂为什么
    addr.sin_addr.S_un.S_addr = ADDR_ANY;   //绑定本地任意IP地址
    if(SOCKET_ERROR == bind(fd, (struct sockaddr*)&addr, sizeof(addr))){    //绑定失败
        err("bind()");
        return INVALID_SOCKET; 
    }
    //开始监听
    listen(fd, 10);
    printf("\n服务器启动成功...\n");
    return fd;
}

SOCKET connect_clientSocket(SOCKET serfd)
{
    SOCKET clifd = accept(serfd, NULL, NULL);
    if(clifd == INVALID_SOCKET){    //连接失败
        err("accept()");
    }
    printf("\n连接成功...\n");
    return clifd;
}




//服务器主函数
int main()
{
    //创建服务器
    SOCKET serfd = create_serverSocket();
    printf("\n服务器创建成功，等待客户端连接中...\n");
}