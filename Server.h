#ifndef SNMP_SERVER_H
#define SNMP_SERVER_H

#include "Common.h"
using namespace std;
static int count;

//服务端类, 用来处理客户端的请求
class Server {

    public:
        Server();

        void Init();

        void Close();

        void Start();
    private:
        //发送文件
        void sendFile(char *filePath);

        char *GetFileName(char *path);

        int get_size(FILE *fp);

        Msg sendMsg;
        Msg recvMsg;

        int server_fd;

        char send_line[MAXLINE], recv_line[MAXLINE + 1];

        struct sockaddr_in server_addr;

        int clients_list[MAX_CLIENT] = {0};

        struct sockaddr_in client_addr;

        socklen_t client_len = sizeof(client_addr);

        //处理新用户的连接
        void welcomeClient();

        void dealCmd();

        //用户在服务端远程控制客户端, 接受客户端的文件
        void recvFile();

        void Nothing();
};
#endif