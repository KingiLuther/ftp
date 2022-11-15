#include <stdio.h>
#include <stdbool.h>
#include <winsock2.h>
#pragma comment(lib, "ws2_32.lib")




//与客户端连接/断开连接

bool init_Socket(){
    WSADATA wsadata; 
    if(0 != WSAStartup(MAKEWORD(2, 2), &wsadata)){  //windows异步套接字启动失败
        err("WSAStartup()");
        return false;
    }
    return true;
}





//服务器主函数
int main()
{

}