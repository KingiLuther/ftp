#include <stdio.h>
#include <string.h>  
#include <stdbool.h>
#include <direct.h>
#include <stdlib.h>
#include <winsock2.h>
#pragma comment(lib, "ws2_32.lib")

//报错信息
#define err(errMsg) printf("[error]%s failed! code: %d line: %d", errMsg, WSAGetLastError(), __LINE__);
//端口信息
//#define PORT 25742
#define PORT 6798
#define fake_IP "103.205.254.211"
#define MAXLINE 1024

/*函数声明*/
bool init_Socket();
bool close_Socket();
SOCKET create_clientSocket(const char* ip);
SOCKET connect_serverSocket();
bool send_Msg(SOCKET serfd, char* sendBuf);
bool recv_Msg(SOCKET serfd, char* recvBuf);
bool show_Msg(SOCKET serfd, char* recvBuf);
bool isASCII(const char *filename);
bool recv_File_A(SOCKET serfd, char* token);
bool recv_File_B(SOCKET serfd, char* token);
bool send_File_A(SOCKET serfd, char *filename);
bool send_File_B(SOCKET serfd, char *filename);
bool user_Login();


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
    addr.sin_port = htons(PORT);            //端口号，用htons把本地字节序转为网络字节序，不懂为什么
    addr.sin_addr.S_un.S_addr = inet_addr(ip);   //绑定服务器IP地址
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
    //SOCKET serfd = create_clientSocket(fake_IP);
    if(serfd == INVALID_SOCKET)
        printf("Connection failed...");
    else printf("\nConnection succeeded...\n");
    return serfd;
}

/*向服务器发送信息*/
bool send_Msg(SOCKET serfd, char* sendBuf)
{
    //1.发送sendBuf长度
    char len[MAXLINE] = "";
    itoa(strlen(sendBuf), len, 10);
    if(SOCKET_ERROR == send(serfd, len, strlen(len), 0)){ //发送失败
        err("send()");
        return false;
    }

    //2.发送sendBuf
    if(SOCKET_ERROR == send(serfd, sendBuf, strlen(sendBuf), 0)){ //发送失败
        err("send()");
        return false;
    }

    //3.告诉服务器发送完成
    send(serfd, "DONE", 4, 0);

    return true;
}

/*接收服务器信息*/
bool recv_Msg(SOCKET serfd, char* recvBuf)  //只接受，不展示
{
    //0.清空recvBuf
    memset(recvBuf, 0, MAXLINE);
    //1.接收长度
    char t_len[MAXLINE] = "";
    recv(serfd, t_len, MAXLINE, 0);
    int len = atoi(t_len);
    //2.接收信息
    int ret = recv(serfd, recvBuf, len, 0);
    if(ret == 0){
        printf("\nClient is offline...\n");
        return false;
    }
    else if(ret < 0){   //接收出错
        err("recv()");
        return false;
    }
    return true;
}

bool show_Msg(SOCKET serfd, char* recvBuf)  //展示
{
    //0.清空recvBuf
    memset(recvBuf, 0, MAXLINE);
    //1.接收长度
    char t_len[MAXLINE] = "";
    recv(serfd, t_len, MAXLINE, 0);
    int len = atoi(t_len);
    //2.接收信息
    int ret = recv(serfd, recvBuf, len, 0);
    if(ret == 0){
        printf("\nClient is offline...\n");
        return false;
    }
    else if(ret < 0){   //接收出错
        err("recv()");
        return false;
    }
    puts(recvBuf);
    return true;
}


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

bool recv_File_A(SOCKET serfd, char* token)
{
    char* fileName = token; 

    //1.接收文件长度
    long g_fileSize = 0;
    char temp[MAXLINE] = "";
    recv_Msg(serfd, temp);
    g_fileSize = atoi(temp);
    printf("File: %s, Size: %ld\n", fileName, g_fileSize);

    //2.打开文件
    FILE* write = fopen(fileName, "w");
    if(!write){
        printf("Fail to open file.");
        return false;
    }

    //3.接收文件本体
    int pktNum = (g_fileSize%MAXLINE == 0) ? (g_fileSize/MAXLINE) : ((g_fileSize/MAXLINE) + 1);
    int ret = 0;
    char tempBuf[MAXLINE] = "";
    int total = 0;
    //接收前 pktNum-1 个包
    for(int i=1; i<=pktNum-1; i++){
        memset(tempBuf, 0, MAXLINE);
        ret = recv(serfd, tempBuf, MAXLINE, 0);
        //printf("ret = %d\n", ret);
        if(ret == 0){
            printf("\nServer is offline...\n");
            return false;
        }
        else if(ret < 0){   //接收出错
            err("recv()");
            return false;
        }
        fwrite(tempBuf, sizeof(char), MAXLINE, write);
        //printf("get pkt%d\n", i);
        total += ret;
    }
    //接收最后一个包
    memset(tempBuf, 0, MAXLINE);
    int end = g_fileSize % MAXLINE;
    ret = recv(serfd, tempBuf, end, 0);
    total += ret;
    fwrite(tempBuf, sizeof(char), end, write);
    fclose(write);

    //5.接收完成
    memset(tempBuf, 0, MAXLINE);
    recv(serfd, tempBuf, MAXLINE, 0);
    if(strcmp(tempBuf, "DONE") == 0){   //接收成功
        printf("Download succeeded, Succeed to receive %d Byte...\n", total);
        return true;
    }
    printf("Fail to download.\n");
    remove(fileName);
    return false;

}

bool recv_File_B(SOCKET serfd, char* token)
{
    char* fileName = token; 

    //1.接收文件长度
    long g_fileSize = 0;
    char temp[MAXLINE] = "";
    recv_Msg(serfd, temp);
    g_fileSize = atoi(temp);
    printf("File: %s, Size: %ld\n", fileName, g_fileSize);

    //2.打开文件
    FILE* write = fopen(fileName, "wb");
    if(!write){
        printf("Fail to open file.");
        return false;
    }

    //3.接收文件本体
    int pktNum = (g_fileSize%MAXLINE == 0) ? (g_fileSize/MAXLINE) : ((g_fileSize/MAXLINE) + 1);
    int ret = 0;
    char tempBuf[MAXLINE] = "";
    int total = 0;
    //接收前 pktNum-1 个包
    for(int i=1; i<=pktNum-1; i++){
        memset(tempBuf, 0, MAXLINE);
        ret = recv(serfd, tempBuf, MAXLINE, 0);
        //printf("ret = %d\n", ret);
        if(ret == 0){
            printf("\nServer is offline...\n");
            return false;
        }
        else if(ret < 0){   //接收出错
            err("recv()");
            return false;
        }
        fwrite(tempBuf, sizeof(char), MAXLINE, write);
        //printf("get pkt%d\n", i);
        total += ret;
    }
    //接收最后一个包
    memset(tempBuf, 0, MAXLINE);
    int end = g_fileSize % MAXLINE;
    ret = recv(serfd, tempBuf, end, 0);
    total += ret;
    fwrite(tempBuf, sizeof(char), end, write);
    fclose(write);

    //5.接收完成
    memset(tempBuf, 0, MAXLINE);
    recv(serfd, tempBuf, MAXLINE, 0);
    if(strcmp(tempBuf, "DONE") == 0){   //接收成功
        printf("Download succeeded, Succeed to receive %d Byte...\n", total);
        return true;
    }
    printf("Fail to download.\n");
    remove(fileName);
    return false;

}

bool send_File_A(SOCKET serfd, char* filename)
{
    FILE* fp = fopen(filename, "r");
    if(!fp){
        printf("Fail to open file.\n");
        return false;
    }
    //获取文件大小
    fseek(fp, 0, SEEK_END);   //指针指向文件最后
    long g_fileSize = ftell(fp);
    fseek(fp, 0, SEEK_SET);   //指针指向文件开始
    //把文件读到内存中来
    char* g_fileBuf = calloc(g_fileSize, sizeof(char));   //申请内存
    if(!g_fileBuf)  //内存申请失败
        return false;
    fread(g_fileBuf, sizeof(char), g_fileSize, fp);
    fclose(fp);

    //发送文件
    //1.发送文件长度
    char t_fileSize[MAXLINE] = "";
    itoa(g_fileSize, t_fileSize, 10);
    send_Msg(serfd, t_fileSize);
    printf("File:%d B\n", g_fileSize);

    //2.发送文件本体
    int ret = send(serfd, g_fileBuf, g_fileSize, 0);
    printf("ret = %d\n", ret);
    if(ret == SOCKET_ERROR){    //文件发送失败
        err("send()");
        return false;
    }

    //3.告诉客户端发送完成
    send(serfd, "DONE", 4, 0);
    printf("\nSucceed to send %d Byte...\n", ret);
    return true;
}

bool send_File_B(SOCKET serfd, char* filename)
{
    FILE* fp = fopen(filename, "rb");
    if(!fp){
        printf("Fail to open file.\n");
        return false;
    }
    //获取文件大小
    fseek(fp, 0, SEEK_END);   //指针指向文件最后
    long g_fileSize = ftell(fp);
    fseek(fp, 0, SEEK_SET);   //指针指向文件开始
    //把文件读到内存中来
    char* g_fileBuf = calloc(g_fileSize, sizeof(char));   //申请内存
    if(!g_fileBuf)  //内存申请失败
        return false;
    fread(g_fileBuf, sizeof(char), g_fileSize, fp);
    fclose(fp);

    //发送文件
    //1.发送文件长度
    char t_fileSize[MAXLINE] = "";
    itoa(g_fileSize, t_fileSize, 10);
    send_Msg(serfd, t_fileSize);
    printf("File:%d B\n", g_fileSize);

    //2.发送文件本体
    int ret = send(serfd, g_fileBuf, g_fileSize, 0);
    printf("ret = %d\n", ret);
    if(ret == SOCKET_ERROR){    //文件发送失败
        err("send()");
        return false;
    }
    
    //3.告诉客户端发送完成
    send(serfd, "DONE", 4, 0);
    printf("\nSucceed to send %d Byte...\n", ret);
    return true;
}

/*用户登录*/
bool user_Login()
{
    char userName[128] = "";
    char userPass[128] = "";
    printf("\nPlease enter the user name: ");
    gets(userName);
    printf("\nPlease enter the password: ");
    gets(userPass);
    printf("\nLogin succeeded. Welcome, %s.\n", userName);
    return true;
}



/*客户端主函数*/
int main()
{
    //启动客户端
    init_Socket();
    SOCKET serfd = connect_serverSocket();

    //匿名登录
    user_Login();

    //test
    while(true){

            printf("\nftp>");   //伪装命令行
            char sendBuf[MAXLINE] = "";
            gets(sendBuf);
            //拆分指令
            char command[MAXLINE] = "";
            char token[MAXLINE] = "";
            sscanf(sendBuf, "%s %s", command, token);
            //根据指令进行后续操作
            char recvBuf[MAXLINE] = "";
            if(command != NULL)
            {
                if(strcmp("quit", command) == 0){
                    send_Msg(serfd, sendBuf);
                    printf("Good bye!");
                    break;
                }
                else if(strcmp("help", command) == 0){
                    printf("    get [remote_filename]\n");
                    printf("    put [local_filename]\n");
                    printf("    delete [remote_filename]\n");
                    printf("    cd [remote_direcotry_name] or cd ..\n");
                    printf("    mkdir [remote_direcotry_nam]\n");
                    printf("    ls\n");
                    printf("    pwd\n");
                    printf("    help\n");
                    printf("    quit\n");
                }   
                else if(strcmp("pwd", command) == 0){
                    send_Msg(serfd, sendBuf);
                    show_Msg(serfd, recvBuf);
                }    
                else if(strcmp("ls", command) == 0){
                    send_Msg(serfd, sendBuf);
                    show_Msg(serfd, recvBuf);
                }
                else if(strcmp("cd", command) == 0){
                    send_Msg(serfd, sendBuf);
                    show_Msg(serfd, recvBuf);
                }
                else if(strcmp("mkdir", command) == 0){
                    send_Msg(serfd, sendBuf);
                    show_Msg(serfd, recvBuf);
                }
                else if(strcmp("delete", command) == 0){
                    send_Msg(serfd, sendBuf);
                    show_Msg(serfd, recvBuf);
                }
                else if(strcmp("get", command) == 0){
                    send_Msg(serfd, sendBuf);
                    recv_Msg(serfd, recvBuf);
                    if(strcmp("ASCII", recvBuf) == 0){
                        recv_File_A(serfd, token);
                    }
                    else if(strcmp("Binary", recvBuf) == 0){
                        recv_File_B(serfd, token);
                    }
                }
                else if(strcmp("put", command) == 0){
                    send_Msg(serfd, sendBuf);
                    if(token){
                        if(isASCII(token) == true){ //文件是ASCII
                            send_Msg(serfd, "ASCII");
                            send_File_A(serfd, token);
                        }
                        else{   //文件是二进制
                            send_Msg(serfd, "Binary");
                            send_File_B(serfd, token);
                        }
                    }
                    else   //用户输入的get后面没有参数
                        printf("Please enter the name of the file.");
                }
                else{   //用户输入有误
                    printf("Your input is incorrect, please enter 'help' for more information.\n");
                }       
            }

    }

    //关闭一切
    closesocket(serfd);
    close_Socket();
    getchar();
    return 0;

}