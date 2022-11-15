#include <stdio.h>
#include <string.h>  
#include <stdbool.h>
#include <direct.h>
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
    printf("\n服务器创建成功，等待客户端连接中...\n");
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


/*向客户端发送信息*/
bool send_Msg(SOCKET clifd, char* sendBuf)
{
    if(SOCKET_ERROR == send(clifd, sendBuf, strlen(sendBuf), 0)){ //发送失败
        err("send()");
        return false;
    }
    return true;
}

/*接收客户端信息*/
bool recv_Msg(SOCKET clifd, char* recvBuf)
{
    int ret = recv(clifd, recvBuf, 1024, 0);
    if(ret == 0){
        printf("\n客户端正常下线...\n");
        return true;
    }
    else if(ret < 0){   //接收出错
        err("recv()");
        return false;
    }
    puts(recvBuf);

    //拆分指令
    char* t_recvBuf = recvBuf;
    char* command = strtok(t_recvBuf, " ");
    //printf(t_recvBuf, command); 原来的，分割后的
    if(command != NULL)
    {
        if(strcmp("pwd", t_recvBuf) == 0)
            ftp_pwd(clifd);
        else if(strcmp("ls", t_recvBuf) == 0)
            ftp_ls(clifd);
            

    }


    return true;
}


/*指令实现*/
void ftp_pwd(SOCKET clifd)
{
    char* path;
    path = _getcwd(NULL, 0);
    if (path != 0){
        char sendBuf[1024] = "";
        strcpy(sendBuf, path);
        send_Msg(clifd, sendBuf);
    };
}

void ftp_ls(SOCKET clifd)
{
    char* path;
    path = _getcwd(NULL, 0);
    if (path != 0){
        char sendBuf[1024] = "";
        strcpy(sendBuf, path);
        send_Msg(clifd, sendBuf);
    };
}



//服务器主函数
int main()
{
    //启动服务器
    init_Socket();
    SOCKET serfd = create_serverSocket();
    //与客户端连接
    SOCKET clifd = connect_clientSocket(serfd);


    //test
    while(true){
        char recvBuf[1024] = "";
        recv_Msg(clifd, recvBuf);
        break;
    }




    //关闭一切
    closesocket(clifd);
    closesocket(serfd);
    close_Socket();
    getchar();
    return 0;

}