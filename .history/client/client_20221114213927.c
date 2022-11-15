#include <stdio.h>
#include <string.h>  
#include <stdbool.h>
#include <direct.h>
#include <stdlib.h>
#include <winsock2.h>
#pragma comment(lib, "ws2_32.lib")

//报错信息
#define err(errMsg) printf("[error]%s failed! line: %d", errMsg, __LINE__);
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

/*接收服务器信息*/
bool recv_Msg(SOCKET serfd, char* recvBuf)
{
    int ret = recv(serfd, recvBuf, 1024, 0);
    if(ret == 0){
        printf("\n客户端正常下线...\n");
        return false;
    }
    else if(ret < 0){   //接收出错
        err("recv()");
        return false;
    }
    puts(recvBuf);
    return true;
}

// /*指令实现*/
// bool ftp_pwd()
// {
//     char* path;
//     path = _getcwd(NULL, 0);
//     if (path != 0){
//         char sendBuf[1024] = "";
//         strcpy_s(sendBuf, path);
//         send_Msg(clifd, sendBuf);
//     };
//     return true;
// }

/*文件操作*/

bool isASCII(const char *filename)
{
    FILE *fp = fopen(filename, "rb");
    long file_length;
    long A_count = 0;
    int read_len;
    unsigned char byte;
    fseek(fp, 0, SEEK_END);
    file_length = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    while ((read_len = fread(&byte, 1, 1, fp)) > 0){
        if (byte >= 7)
            A_count++;
    }
    fclose(fp);
    return (A_count / file_length > 0.8) ? true : false;
}

bool recv_File_A(SOCKET serfd)
{
    //1.接收文件长度
    long g_fileSize = 0;
    char temp[1024];

    //2.接收文件名
    char fileName[1024];
    recv_Msg(serfd, fileName);
    //3.接收文件本体
    char* g_fileBuf = calloc(g_fileSize, sizeof(char));   //申请内存
    if(!g_fileBuf)  //内存申请失败
        return false;
    int ret = recv(serfd, g_fileBuf, g_fileSize, 0);
    if(ret == 0){
        printf("\n服务器正常下线...\n");
        return false;
    }
    else if(ret < 0){   //接受出错
        err("recv()");
        return false;
    }
    printf("\n下载成功...\n");
    FILE* write = fopen(fileName, "wb");
    if(!write){
        err("fopen()");
        return false;
    }
    fwrite(g_fileBuf, sizeof(char), g_fileSize, write);
    printf("保存成功\n");
    fclose(write);
    return true;
}












/*客户端主函数*/
int main()
{
    //启动客户端
    init_Socket();
    SOCKET serfd = connect_serverSocket();

    //test
    while(true){
        printf("\nftp>");   //伪装命令行
        char sendBuf[1024] = "";
        gets(sendBuf);
        send_Msg(serfd, sendBuf);

        //发送了什么命令呢？
        char* command = strtok(sendBuf, " ");
        //根据指令进行后续操作
        if(command != NULL)
        {
            if(strcmp("quit", command) == 0){
                printf("Good bye!");
                break;
            }
            else if(strcmp("pwd", command) == 0){
                char recvBuf[1024] = "";
                recv_Msg(serfd, recvBuf);
            }    
            else if(strcmp("ls", command) == 0){

            }
            else if(strcmp("cd", command) == 0){
                char recvBuf[1024] = "";
                recv_Msg(serfd, recvBuf);
            }
            else if(strcmp("mkdir", command) == 0){
                char recvBuf[1024] = "";
                recv_Msg(serfd, recvBuf);
            }
            else if(strcmp("delete", command) == 0){
                char recvBuf[1024] = "";
                recv_Msg(serfd, recvBuf);
            }        
        }
    }





    //关闭一切
    closesocket(serfd);
    close_Socket();
    getchar();
    return 0;

}