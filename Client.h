#ifndef SNMP_CLIENT_H
#define SNMP_CLIENT_H

#include "Common.h"

using namespace std;

//客户端类
class Client {
    public:
        Client();

        void Connect();

        void Close();

        void Start();

        //运行从服务端接收到的cmd命令
        char* run_cmd(char *cmd);

        //发送文件
        void sendFile(char *filePath);

        void recvFile();
    private:
        // 表示客户端是否正常工作
        bool isClientwork;

        // epoll_create创建后的返回值
        int epfd;

        // 与服务端通信传输的信息
        Msg msg;

        int socket_fd;

        struct sockaddr *reply_addr;

        socklen_t len;

        char send_line[MAXLINE], recv_line[MAXLINE + 1];

        //用户连接的服务器 IP + port
        struct sockaddr_in server_addr;
        socklen_t server_len;

        void dealCmd(char *cmd);

        char *GetFileName(char *path);

        int get_size(FILE *fp);

        Msg sendMsg, recvMsg;
};

#endif