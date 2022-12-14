#include <stdio.h>
#include <string.h>  
#include <stdbool.h>
#include <direct.h>
#include <io.h>
#include <stdlib.h>
#include <winsock2.h>
#pragma comment(lib, "ws2_32.lib")

//报错信息
#define err(errMsg) printf("[error]%s failed! line: %d", errMsg, __LINE__);
//端口信息
#define PORT 6798

//函数声明
bool init_Socket();
bool close_Socket();
SOCKET create_serverSocket();
SOCKET connect_clientSocket(SOCKET serfd);
bool send_Msg(SOCKET clifd, char* sendBuf);
bool ftp_pwd(SOCKET clifd);
bool ftp_ls(SOCKET clifd);
bool ftp_quit(SOCKET clifd);
bool ftp_cd(SOCKET clifd, char* token);
bool ftp_mkdir(SOCKET clifd, char* token);
bool ftp_delete(SOCKET clifd, char* token);
bool ftp_get(SOCKET clifd, char* token);
bool ftp_put(SOCKET clifd, char* token);
int isASCII(const char *filename);
bool send_File_A(SOCKET clifd, char* filename);
bool send_File_B(SOCKET clifd, char* filename);
bool recv_File_A(SOCKET clifd, char* token);
bool recv_File_B(SOCKET clifd, char* token);
bool recv_Msg(SOCKET clifd, char* recvBuf);
bool recv_Cmd(SOCKET clifd, char* recvBuf);

#define MAXLINE 1024

/*与客户端连接/断开连接*/
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
    SOCKET fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);  //使用TCP协议簇
    if(INVALID_SOCKET == fd){   //创建失败
        err("socket()");
        return INVALID_SOCKET;
    }
    //给socket绑定本地IP地址和端口号
    struct sockaddr_in addr;                //包含端口号、IP地址等信息的结构体
    addr.sin_family = AF_INET;              //协议族需要与前面的协议族相同，不懂为什么
    addr.sin_port = htons(PORT);            //用htons把本地字节序转为网络字节序，不懂为什么
    addr.sin_addr.S_un.S_addr = ADDR_ANY;   //绑定本地IP地址
    if(SOCKET_ERROR == bind(fd, (struct sockaddr*)&addr, sizeof(addr))){    //绑定失败
        err("bind()");
        return INVALID_SOCKET; 
    }
    //开始监听
    listen(fd, 1);
    printf("\nWait for client to connect...\n");
    return fd;
}

SOCKET connect_clientSocket(SOCKET serfd)
{
    SOCKET clifd = accept(serfd, NULL, NULL);
    if(clifd == INVALID_SOCKET){    //连接失败
        err("accept()");
    }
    printf("\nConnection succeeded...\n");
    return clifd;
}


/*向客户端发送信息*/
bool send_Msg(SOCKET clifd, char* sendBuf)
{
    //1.发送sendBuf长度
    char len[MAXLINE] = "";
    itoa(strlen(sendBuf), len, 10);
    if(SOCKET_ERROR == send(clifd, len, strlen(len), 0)){ //发送失败
        err("send()");
        return false;
    }
    //2.发送sendBuf
    if(SOCKET_ERROR == send(clifd, sendBuf, strlen(sendBuf), 0)){ //发送失败
        err("send()");
        return false;
    }
    return true;
}

/*指令实现*/
bool ftp_pwd(SOCKET clifd)
{
    char* path;
    path = _getcwd(NULL, 0);
    if (path != 0){
        char sendBuf[MAXLINE] = "";
        strcpy(sendBuf, path);
        send_Msg(clifd, sendBuf);
        printf("ftp>do pwd\n");
        return true;
    };
    return false;
}

bool ftp_ls(SOCKET clifd)
{
    char* path = _getcwd(NULL, 0);
    struct _finddata_t data;
    char mode[10] = "\\*.*";
	sprintf(path, "%s%s", path, mode);
    long HANDLE = _findfirst(path, &data);
    if (HANDLE < 0){
    	printf("Fail to _finefirst()");
        return false;
	}
    int nRet = 1;
    char sendBuf[MAXLINE] = "";
    while (nRet >= 0)
    {
        sprintf(sendBuf, "%s    %s\n", sendBuf, data.name);
        nRet = _findnext(HANDLE, &data );
    }
    _findclose(HANDLE);
    send_Msg(clifd, sendBuf);
    printf("ftp>do ls\n");
    return true;
}

bool ftp_quit(SOCKET clifd)
{
    printf("\nBye!\n");
    return true;
}

bool ftp_cd(SOCKET clifd, char* token)
{
    if(token){
        if(_chdir(token) < 0){  //工作路径切换失败
            send_Msg(clifd, "Directory doesn't exist.");
            return false;
        }
        else { //工作路径切换成功
            send_Msg(clifd, "Directory successfully changed.");
            printf("ftp>do cd\n");
            return true;
        }
    }
    else{   //用户输入的cd后面没有参数
        send_Msg(clifd, "Please enter the directory.");
        return false;
    }
}

bool ftp_mkdir(SOCKET clifd, char* token)
{
    if(token){
        if(mkdir(token) < 0){  //文件夹创建失败
            send_Msg(clifd, "Fail to create folder.");
            return false;
        }
        else{ //文件夹创建成功
            send_Msg(clifd, "Folder successfully created.");
            printf("ftp>do mkdir\n");
            return true;
        }
    }
    else{  //用户输入的mkdir后面没有参数
        send_Msg(clifd, "Please enter the name of the folder.");
        return false;
    }
}

bool ftp_delete(SOCKET clifd, char* token)
{
    if(token){
        if(remove(token) < 0){  //文件删除失败
            send_Msg(clifd, "Fail to delete the file.");
            return false;
        }
        else{  //文件删除成功
            send_Msg(clifd, "File successfully deleted.");
            printf("ftp>do delete\n");
            return true;
        }
    }
    else{   //用户输入的delete后面没有参数
        send_Msg(clifd, "Please enter the name of the file.");
        return false;
    }
}

bool ftp_get(SOCKET clifd, char* token)
{
    if(token){
        if(isASCII(token) == 1){ //文件是ASCII
            printf("\nThe file is ASCII...\n");
            send_Msg(clifd, "ASCII");
            send_File_A(clifd, token);
            printf("ftp>do get\n");
            return true;
        }
        else if(isASCII(token) == 2){   //文件是二进制
            printf("\nThe file is binary...\n");
            send_Msg(clifd, "Binary");
            send_File_B(clifd, token);
            printf("ftp>do get\n");
            return true;
        }
        else{
            printf("\nThe file doesn't exist.\n");
            send_Msg(clifd, "Wrong");
            printf("ftp>do get\n");
            return true;
        }
    }
    else{   //用户输入的get后面没有参数
        send_Msg(clifd, "Please enter the name of the file.");
        return false;
    }
}

bool ftp_put(SOCKET clifd, char* token)
{
    if(token){
        char Buf[MAXLINE] = "";
        recv_Msg(clifd, Buf);
        if(strcmp("ASCII", Buf) == 0){
            if(!recv_File_A(clifd, token)){
                printf("Fail to download.\n");
                return false;
            }
            else{
                printf("ftp>do put\n");
                return true;
            }
        }
        else if(strcmp("Binary", Buf) == 0){
            if(!recv_File_B(clifd, token)){
                printf("Fail to download.\n");
                return false;
            }
            else{
                printf("ftp>do put\n");
                return true;
            }
        }
    }
    else{   //用户输入的put后面没有参数
        printf("Please enter the name of the file.");
        return false;
    }
}

/*文件操作*/

int isASCII(const char *filename)   //1,ASCII 2,Binary, -1,False
{
    FILE *fp = fopen(filename, "rb");
    if(!fp){
        printf("Fail to open file, please cheack the name of file.\n");
        return -1;
    }
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
    return (A_count / file_length > 0.8) ? 1 : 2;
}

bool send_File_A(SOCKET clifd, char* filename)
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
    send_Msg(clifd, t_fileSize);
    printf("File:%d B\n", g_fileSize);

    //2.发送文件本体
    int ret = send(clifd, g_fileBuf, g_fileSize, 0);
    printf("ret = %d\n", ret);
    if(ret == SOCKET_ERROR){    //文件发送失败
        err("send()");
        return false;
    }

    //3.告诉客户端发送完成
    send(clifd, "DONE", 4, 0);
    printf("\nSucceed to send %d Byte...\n", ret);
    return true;
}

bool send_File_B(SOCKET clifd, char* filename)
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
    send_Msg(clifd, t_fileSize);
    printf("File:%d B\n", g_fileSize);

    //2.发送文件本体
    int ret = send(clifd, g_fileBuf, g_fileSize, 0);
    printf("ret = %d\n", ret);
    if(ret == SOCKET_ERROR){    //文件发送失败
        err("send()");
        return false;
    }
    
    //3.告诉客户端发送完成
    send(clifd, "DONE", 4, 0);
    printf("\nSucceed to send %d Byte...\n", ret);
    return true;
}

bool recv_File_A(SOCKET clifd, char* token)
{
    char* fileName = token; 

    //1.接收文件长度
    long g_fileSize = 0;
    char temp[MAXLINE] = "";
    if(!recv_Msg(clifd, temp)){
        printf("Fail to receive the length of file.\n");
        return false;
    }
    g_fileSize = atoi(temp);
    printf("File: %s, Type: ASCII, Size: %ld\n", fileName, g_fileSize);

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
        ret = recv(clifd, tempBuf, MAXLINE, 0);
        if(ret == 0){
            printf("\nServer is offline...\n");
            return false;
        }
        else if(ret < 0){   //接收出错
            err("recv()");
            return false;
        }
        fwrite(tempBuf, sizeof(char), MAXLINE, write);
        total += ret;
    }
    //接收最后一个包
    memset(tempBuf, 0, MAXLINE);
    int end = g_fileSize % MAXLINE;
    ret = recv(clifd, tempBuf, end, 0);
    total += ret;
    fwrite(tempBuf, sizeof(char), end, write);
    fclose(write);

    //5.接收完成
    memset(tempBuf, 0, MAXLINE);
    recv(clifd, tempBuf, MAXLINE, 0);
    if(strcmp(tempBuf, "FILE") == 0){   //接收成功
        printf("Download succeeded, Succeed to receive %d Byte...\n", total);
        return true;
    }
    printf("Fail to download.\n");
    remove(fileName);
    return false;

}

bool recv_File_B(SOCKET clifd, char* token)
{
    char* fileName = token; 

    //1.接收文件长度
    long g_fileSize = 0;
    char temp[MAXLINE] = "";
    if(!recv_Msg(clifd, temp)){
        printf("Fail to receive the length of file.\n");
        return false;
    }
    g_fileSize = atoi(temp);
    printf("File: %s, Type: ASCII, Size: %ld\n", fileName, g_fileSize);

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
        ret = recv(clifd, tempBuf, MAXLINE, 0);
        if(ret == 0){
            printf("\nServer is offline...\n");
            return false;
        }
        else if(ret < 0){   //接收出错
            err("recv()");
            return false;
        }
        fwrite(tempBuf, sizeof(char), MAXLINE, write);
        total += ret;
    }
    //接收最后一个包
    memset(tempBuf, 0, MAXLINE);
    int end = g_fileSize % MAXLINE;
    ret = recv(clifd, tempBuf, end, 0);
    total += ret;
    fwrite(tempBuf, sizeof(char), end, write);
    fclose(write);

    //5.接收完成
    memset(tempBuf, 0, MAXLINE);
    recv(clifd, tempBuf, MAXLINE, 0);
    if(strcmp(tempBuf, "FILE") == 0){   //接收成功
        printf("Download succeeded, Succeed to receive %d Byte...\n", total);
        return true;
    }
    printf("Fail to download.\n");
    remove(fileName);
    return false;

}

/*接收客户端信息*/
bool recv_Msg(SOCKET clifd, char* recvBuf)  //只接受，不展示
{
    //0.清空recvBuf
    memset(recvBuf, 0, MAXLINE);
    //1.接收长度
    char t_len[MAXLINE] = "";
    recv(clifd, t_len, MAXLINE, 0);
    int len = atoi(t_len);
    //2.接收信息
    int ret = recv(clifd, recvBuf, len, 0);
    if(ret == 0){
        printf("\nClient is offline...\n");
        return false;
    }
    else if(ret < 0){   //接收出错
        err("recv()");
        return false;
    }
    //3.接收完成
    char tempBuf[MAXLINE] = "";
    memset(tempBuf, 0, MAXLINE);
    recv(clifd, tempBuf, MAXLINE, 0);
    if(strcmp(tempBuf, "DONE") == 0){   //接收成功
        return true;
    }
    return false;
}


/*接收客户端指令*/
bool recv_Cmd(SOCKET clifd, char* recvBuf)
{
    //0.清空recvBuf
    memset(recvBuf, 0, MAXLINE);
    //1.接收指令长度
    char t_len[MAXLINE] = "";
    recv(clifd, t_len, MAXLINE, 0);
    int len = atoi(t_len);
    //2.接收指令
    int ret = recv(clifd, recvBuf, len, 0);
    if(ret == 0){
        printf("\nThe command is null...\n");
        return false;
    }
    else if(ret < 0){   //接收出错
        err("recv()");
        return false;
    }
    //3.接收指令成功
    char tempBuf[MAXLINE] = "";
    memset(tempBuf, 0, MAXLINE);
    recv(clifd, tempBuf, MAXLINE, 0);
    if(strcmp(tempBuf, "DONE") == 0){   //接收成功
        //拆分指令
        char command[MAXLINE] = "";
        char token[MAXLINE] = "";
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
            else if(strcmp("put", command) == 0)
                ftp_put(clifd, token);
            else if(strcmp("get", command) == 0)
                ftp_get(clifd, token);
        }
        return true;
    }
    else{
        err("recv()");
        return false;
    }
}


//服务器主函数
int main()
{
    //启动服务器，与客户端连接
    init_Socket();
    SOCKET serfd = create_serverSocket();
    SOCKET clifd = connect_clientSocket(serfd);

    //开始通信
    char nameBuf[MAXLINE] = "";
    if(recv_Msg(clifd, nameBuf))
        printf("%s is connecting...\n", nameBuf);
    else{
        printf("Server is offline!\n");
        closesocket(clifd);
        closesocket(serfd);
        close_Socket();
        getchar();
        return 0;
    }

    //main
    char recvBuf[MAXLINE] = "";
    while(true){
        if(!recv_Cmd(clifd, recvBuf))
            break;
    }

    //关闭服务器
    printf("Server is offline!\n");
    closesocket(clifd);
    closesocket(serfd);
    close_Socket();
    getchar();
    return 0;
}