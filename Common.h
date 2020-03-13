#ifndef SNMP_COMMON_H
#define SNMP_COMMON_H

#include <signal.h>
#include <string>
#include <iostream>
#include <list>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <queue>

// 默认服务器端IP地址
#define SERVER_IP "127.0.0.1"
// 服务器端口号
#define SERVER_PORT 8905
#define    MAXLINE        4096
#define    UNIXSTR_PATH   "/var/lib/unixstream.sock"
#define    LISTENQ        1024
#define    BUFFER_SIZE    4096
#define    MAX_CLIENT     65536
 
// 缓冲区大小65535
#define BUF_SIZE 0xFFFF
#define MAX_SIZE 256

// int epoll_create(int size)中的size
// 为epoll支持的最大句柄数
#define EPOLL_SIZE 5000

// 新用户登录后的欢迎信息
#define SERVER_WELCOME "欢迎你使用SNMP远程管理系统! 您的ID是: Client #%d\n"

//下面定义状态
#define SLEEP 0 //告诉客户端, 你没什么事干, 先睡一会
#define EXCUTE_CMD 1 //告诉客户端执行CMD命令
#define NEW_CLIENT 2 //告诉服务器, 我是新来的, 收到了这个包就把我注册进去
#define HAS_REGIS 3 //高速客户端, 已经注册成功了, 如果客户端没有收到这个包的话, 会一直重传
#define HEAT_BEAT 4 // 代表心跳包
#define FINISHED 5 //代表客户端已经处理完成了
#define SENDFILE 6 //代表客户端发文件来了
#define RECVFILE 7 //代表客户端收文件

//定义SNMP信息结构，在服务端和客户端之间传送
//SNMP报文格式: 版本号, 团体名, PDU
//PDU类型包含两个字节: 一个是真实的PDU类型, 另一个是后面报文所占的字节总数
//除了PDU类型和PDU长度字段外，后面的每个字段都是BER编码方式: 数据类型, 所占字节数, 实际数据
// #pragma pack(1) 
// typedef struct Msg {
//     //这前面是IP头, UDP头

//     char codeType; //编码类型 1b
//     char len;//消息总长度 1b
//     char version[MAX_SIZE];//版本号 nb
//     char community[15];//团体名 nb
//     char pduType;//pdu类型 1b
//     char pduLen;//pdu长度 1b
//     //get/set首部
//     char requestId[MAX_SIZE]; //请求ID nb
//     char errorState[MAX_SIZE]; //差错状态 nb
//     char errorIndex[MAX_SIZE]; //差错索引 nb
//     char variablePair[MAX_SIZE]; //变量名值对

//     //后面是传输的数据
//     char data[128];//存消息
// } Msg;
// #pragma pack ()

//定义信息结构，在服务端和客户端之间传送
typedef struct Msg {
    int event; //告诉客户端要干什么
    int time; //干完了多久后给服务端
    char data[MAX_SIZE]; //具体的内容
} Msg;

#endif // SNMP_COMMON_H