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

//函数声明
bool init_Socket();
bool close_Socket();
SOCKET create_serverSocket();
SOCKET connect_clientSocket(SOCKET serfd);
bool send_Msg(SOCKET clifd, char* sendBuf);
bool send_File_A(SOCKET clifd, char* filename);
bool recv_Msg(SOCKET clifd, char* recvBuf);
bool send_File_B(SOCKET clifd, char* filename);
bool isASCII(const char *filename);
bool recv_File_A(SOCKET clifd);
bool recv_File_B(SOCKET clifd);





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

void ftp_ls(SOCKET clifd)   //代码是抄的，我还没看懂，打印出来的东西也很奇怪
{
    FILE* in;
    char temp[100];
    if(!(in = _popen("dir", "r")))
        err("_popen()");
    if(in){ 
        while (fgets(temp, sizeof(temp), in) != NULL)
            send_Msg(clifd, temp);
        _pclose(in);
    }
}

void ftp_quit(SOCKET clifd)
{
    printf("\nBye!\n");
}

void ftp_cd(SOCKET clifd, char* token)
{
    if(token){
        if(_chdir(token) < 0)  //工作路径切换失败
            send_Msg(clifd, "Directory doesn't exist.");
        else  //工作路径切换成功
            send_Msg(clifd, "Directory successfully changed.");
    }
    else   //用户输入的cd后面没有参数
        send_Msg(clifd, "Please enter the directory.");
}

void ftp_mkdir(SOCKET clifd, char* token)
{
    if(token){
        if(mkdir(token) < 0)  //文件夹创建失败
            send_Msg(clifd, "Fail to create folder.");
        else  //文件夹创建成功
            send_Msg(clifd, "Folder successfully creayed.");
    }
    else   //用户输入的mkdir后面没有参数
        send_Msg(clifd, "Please enter the name of the folder.");
}

void ftp_delete(SOCKET clifd, char* token)
{
    if(token){
        if(remove(token) < 0)  //文件删除失败
            send_Msg(clifd, "Fail to delete the file.");
        else  //文件删除成功
            send_Msg(clifd, "File successfully deleted.");
    }
    else   //用户输入的delete后面没有参数
        send_Msg(clifd, "Please enter the name of the file.");
}

void ftp_get(SOCKET clifd, char* token)
{
    if(token){
        if(isASCII(token) == true){ //文件是ASCII
            printf("\n文件是ASCII\n");
            send_Msg(clifd, "ASCII");
            send_File_A(clifd, token);
        }
        else{   //文件是二进制
            printf("\n文件是二进制\n");
            send_Msg(clifd, "Binary");
            send_File_B(clifd, token);
        }
    }
    else   //用户输入的get后面没有参数
        send_Msg(clifd, "Please enter the name of the file.");
}

void ftp_put(SOCKET clifd)
{
    char recvBuf[1024] = "";
    recv_Msg(clifd, recvBuf);
    if(strcmp("ASCII", recvBuf) == 0){
        printf("\n文件为ASCII\n");
        recv_File_A(clifd);
    }
    else if(strcmp("Binary", recvBuf) == 0){
        printf("\n文件为二进制\n");
        recv_File_B(clifd);
    }
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

bool send_File_A(SOCKET clifd, char* filename)
{
    FILE* fp = fopen(filename, "r");
    if(!fp){
        err("fopen()");
        return false;
    }
    //获取文件大小
    fseek(fp, 0, SEEK_END);   //指针指向文件最后
    long g_fileSize = ftell(fp);
    fseek(fp, 0, SEEK_SET);   //指针指向文件开始
    printf("文件大小:%d B\n", g_fileSize);
    //把文件读到内存中来
    char* g_fileBuf = calloc(g_fileSize, sizeof(char));   //申请内存
    if(!g_fileBuf)  //内存申请失败
        return false;
    fread(g_fileBuf, sizeof(char), g_fileSize, fp);
    fclose(fp);
    //发送文件
    //1.发送文件长度
    printf("开始发送文件长度\n");
    char t_fileSize[1024];
    itoa(g_fileSize, t_fileSize, 10);
    send_Msg(clifd, t_fileSize);
    //2.发送文件名
    printf("开始发送文件名\n");
    send_Msg(clifd, filename);
    //3.发送文件本体
    printf("开始发送文件内容\n");
    int ret = send(clifd, g_fileBuf, g_fileSize, 0);
    if(ret == SOCKET_ERROR){    //文件发送失败
        err("send()");
        return false;
    }
    printf("\n发送成功，文件共 %d 字节...\n", ret);
    return true;
}


bool send_File_B(SOCKET clifd, char* filename)
{
    FILE* fp = fopen(filename, "rb");
    if(!fp){
        err("fopen()");
        return false;
    }
    //获取文件大小
    fseek(fp, 0, SEEK_END);   //指针指向文件最后
    long g_fileSize = ftell(fp);
    fseek(fp, 0, SEEK_SET);   //指针指向文件开始
    printf("文件大小:%d B\n", g_fileSize);
    //把文件读到内存中来
    char* g_fileBuf = calloc(g_fileSize, sizeof(char));   //申请内存
    if(!g_fileBuf)  //内存申请失败
        return false;
    fread(g_fileBuf, sizeof(char), g_fileSize, fp);
    fclose(fp);
    //发送文件
    //1.发送文件长度
    printf("开始发送文件长度\n");
    char t_fileSize[1024];
    itoa(g_fileSize, t_fileSize, 10);
    send_Msg(clifd, t_fileSize);
    //2.发送文件名
    printf("开始发送文件名\n");
    send_Msg(clifd, filename);
    //3.发送文件本体
    printf("开始发送文件内容\n");
    int ret = send(clifd, g_fileBuf, g_fileSize, 0);
    if(ret == SOCKET_ERROR){    //文件发送失败
        err("send()");
        return false;
    }
    printf("\n发送成功，文件共 %d 字节...\n", ret);
    return true;
}

bool recv_File_B(SOCKET clifd)
{
    //1.接收文件长度
    long g_fileSize = 0;
    char temp[1024];
    recv_Msg(clifd, temp);
    g_fileSize = atoi(temp);
    //2.接收文件名
    char fileName[1024];
    recv_Msg(clifd, fileName);
    //3.接收文件本体
    char* g_fileBuf = calloc(g_fileSize, sizeof(char));   //申请内存
    if(!g_fileBuf)  //内存申请失败
        return false;
    int ret = recv(clifd, g_fileBuf, g_fileSize, 0);
    if(ret == 0){
        printf("\n服务器正常下线...\n");
        return false;
    }
    else if(ret < 0){   //接收出错
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

bool recv_File_A(SOCKET clifd)
{
    //1.接收文件长度
    long g_fileSize = 0;
    char temp[1024];
    recv_Msg(clifd, temp);
    g_fileSize = atoi(temp);
    //2.接收文件名
    char fileName[1024];
    recv_Msg(clifd, fileName);
    //3.接收文件本体
    char* g_fileBuf = calloc(g_fileSize, sizeof(char));   //申请内存
    if(!g_fileBuf)  //内存申请失败
        return false;
    int ret = recv(clifd, g_fileBuf, g_fileSize, 0);
    if(ret == 0){
        printf("\n服务器正常下线...\n");
        return false;
    }
    else if(ret < 0){   //接收出错
        err("recv()");
        return false;
    }
    printf("\n下载成功...\n");
    FILE* write = fopen(fileName, "w");
    if(!write){
        err("fopen()");
        return false;
    }
    fwrite(g_fileBuf, sizeof(char), g_fileSize, write);
    printf("保存成功\n");
    fclose(write);
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

    //拆分指令
    char command[1024];
    char token[1024];
    sscanf(recvBuf, "%s %s", command, token);

    //根据指令进行后续操作
    if(command != NULL)
    {
        if(strcmp("quit", command) == 0){
            ftp_quit(clifd);
            return false;
        }
        else if(strcmp("pwd", command) == 0)
            ftp_pwd(clifd);
        else if(strcmp("ls", command) == 0)
            ftp_ls(clifd);
        else if(strcmp("cd", command) == 0)
            ftp_cd(clifd, token);
        else if(strcmp("mkdir", command) == 0)
            ftp_mkdir(clifd, token);
        else if(strcmp("delete", command) == 0)
            ftp_delete(clifd, token);
        else if(strcmp("get", command) == 0){
            ftp_get(clifd, token);
        }
        else if(strcmp("put", command) == 0){
            recv_File_B(clifd);
        }
            

    }


    return true;
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
    for(int i=100; i>0; i--){
        char recvBuf[1024] = "";
        recv_Msg(clifd, recvBuf);
    }


    //关闭一切
    closesocket(clifd);
    closesocket(serfd);
    close_Socket();
    getchar();
    return 0;

}