#include "Server.h"

using namespace std;

Server::Server() {
    bzero(&server_addr, sizeof(server_addr));
    // 初始化服务器地址和端口 我一开始是AF_LOCAL, 一直接受不到信息, 后来换成了AF_INET就好了, 神奇...
    server_addr.sin_family = AF_INET; 
    server_addr.sin_port = htons(SERVER_PORT);
    server_addr.sin_addr.s_addr = inet_addr(SERVER_IP);
}

void Server::Init() {
    cout << "正在初始化服务器..." << endl;

    //创建监听socket
    //函数原型: int socket(int domain, int type, int protocol)
    //SOCK_DGRAM对应UDP, AF_LOCAL表示走本地
    server_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if(server_fd < 0) { 
        perror("生成套接字错误!"); 
        exit(-1);
    }

    //创建出来的套接字如果需要被别人使用,就需要调用bind函数把套接字和套接字地址绑定
    //bind(int fd, sockaddr * addr, socklen_t len)
    //bind函数会根据len字段判断传入的参数addr该怎么解析,len字段表示的就是传入的地址⻓度,它是一个可变值。
    int res = bind(server_fd, (struct sockaddr *) &server_addr, sizeof(server_addr));
    if (res < 0) {
        perror("绑定失败!");
        exit(1);
    }

    cout << "服务器初始化成功!" << endl;
}

void Server::Close() {
    //关闭socket
    close(server_fd);
}

char* Server::GetFileName(char *path) {
    char *name = NULL;
    //在字符串 s 中查找字符 c，返回字符 c 第一次在字符串 s 中出现的位置，如果未找到字符 c，则返回 NULL。
	if (NULL != (name = strrchr(path, '/')))
		return name + 1;
	else if (NULL != (name = strrchr(path, '\\')))
		return name + 1;
	else
		return path;
}

int Server::get_size(FILE *fp) {
    int pre_pos, size;

	pre_pos = ftell(fp);
	fseek(fp, 0, SEEK_END);
	size = ftell(fp);
	fseek(fp, pre_pos, SEEK_SET);

	return size;
}

void Server::sendFile(char *filepath) {
    char *filePath = filepath;
	char *fileName = NULL;
	fileName = GetFileName(filePath);
    sendto(server_fd, (char*)&sendMsg, sizeof(Msg), 0,(struct sockaddr *)&client_addr, client_len);

    //首先把文件的名字发过去
	char buffer[BUF_SIZE];
	int sendBytes = sendto(server_fd, fileName, strlen(fileName), 0,
		(struct sockaddr *)&client_addr, client_len);
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

		sendBytes = sendto(server_fd, buffer, readBytes, 0,
			(struct sockaddr *)&client_addr, client_len);
		sentSize += sendBytes;
		printf("sended: %.1f%%\r", ((float)sentSize * 100) / fileSize);
		if (sendBytes < 0) {
			printf("发送失败!\n");
			exit(1);
		}
	}

	fclose(fp);
}

//注意: 是服务端收各个客户端的信息
void Server::recvFile() {
	char buffer[BUF_SIZE];
	int recvBytes;
	FILE *fp;

	// 首先接受这个文件名
	recvBytes = recvfrom(server_fd, buffer, BUF_SIZE, 0,
		(struct sockaddr *)&client_addr, &client_len);
	if (recvBytes < 0) {
		perror("从客户端接受失败!\n");
		exit(1);
	}
	buffer[recvBytes] = '_';
    buffer[recvBytes+1] = '\0';
    //先创建这个文件, buffer是接收的文件名
	fp = fopen(buffer, "wb");
    printf("正在接收文件%s\n", buffer);
	// 从客户端每次接受BUF_SIZE大小的数据块
	while (true) {
		recvBytes = recvfrom(server_fd, buffer, BUF_SIZE, 0,
			(struct sockaddr *)&client_addr, &client_len);
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

//欢迎新用户
void Server::welcomeClient() {
    int clientfd = client_addr.sin_port;
    cout << "客户端连接来自于: "
            << inet_ntoa(client_addr.sin_addr) << ":"
            << client_addr.sin_port << endl;

    // 服务端用list保存用户连接
    clients_list[clientfd]++;
    clients_list[0]++;
    cout << "现在有 " << clients_list[0] << " 个客户端正在被远程控制!" << endl;

    // debug 服务端发送欢迎信息  
    cout << "welcome message" << endl;                
    printf(SERVER_WELCOME, clientfd);
}

void Server::Nothing() {
    char buf[BUF_SIZE];
    char tmp[BUF_SIZE];
    Msg msg;
    memset(&msg, 0, sizeof(Msg));
    msg.event = SLEEP;
    sendto(server_fd, (char*)&msg, sizeof(Msg), 0, (struct sockaddr*)&client_addr, client_len);
}

void Server::Start() {
    //初始化服务端
    Init();
    int fd;
    char cmd[128];
    int event, time;
    //轮循逻辑
    while (1) {
        memset(&sendMsg, 0, sizeof(sendMsg));
        memset(&recvMsg, 0, sizeof(recvMsg));
        //这一次接受轮询查看是否存在新连接的用户
        int cnt = 0;
        while (true) {
            //服务端接受来自客户端的轮询
            int n = recvfrom(server_fd, (char*)&recvMsg, 128, 0, (struct sockaddr *)&client_addr, &client_len);
            event = recvMsg.event;
            time = recvMsg.time;
            if (event == NEW_CLIENT) {
                welcomeClient();
                Nothing();
            } else if (event == FINISHED){
                cout << "客户端: "
                        << inet_ntoa(client_addr.sin_addr) << ":"
                        << client_addr.sin_port << "完成任务!" << endl;
                fputs(recvMsg.data, stdout);
                fputs("\n", stdout);
                printf("================================\n");
                Nothing();
            } else if (event == SENDFILE){
                cout << "客户端: "
                        << inet_ntoa(client_addr.sin_addr) << ":"
                        << client_addr.sin_port << "完成任务!" << endl;
                recvFile();
            } else if (event == RECVFILE) {
                cout << "客户端: "
                        << inet_ntoa(client_addr.sin_addr) << ":"
                        << client_addr.sin_port << "准备接收文件!" << endl;
                sendMsg.event = SENDFILE;
                char target[256];
                bzero(target, sizeof(target));
                memcpy(target, recvMsg.data + 7, strlen(recvMsg.data) - 7);
                sendFile(target);
            } else {
                Nothing();
            }
            cnt++;
            if (cnt == 10) break;
        } 

        if (clients_list[0] != 0) {
            cout << "当前可远程操作的客户端有: ";
            for (int i = 1; i <= (MAX_CLIENT - 1); i++) {
                if (clients_list[i] != 0) {
                    printf("%d ", i);
                }
            }
            cout << endl;
            memset(cmd, 0, sizeof(cmd));
            cout << "请输入你想要远程控制的客户端:" << endl;
            cin >> fd;
            cout << "请输入你想要执行的操作:" << endl;
            getchar();
            fgets(cmd, sizeof(cmd), stdin);
            int i = strlen(cmd);
            if (cmd[i - 1] == '\n') {
                cmd[i - 1] = 0;
            }
        } else {
            cout << "当前无客户端连接, 请等待客户端的连接!" << endl;
            getchar();
            continue;
        }

        if (clients_list[fd] == 0) {
            printf("%d未连接\n", fd);
            continue;
        }

        int count = 0;
        //这一次轮询就是下达任务
        while (true) {
            memset(&recvMsg, 0, sizeof(Msg));
            memset(&sendMsg, 0, sizeof(Msg));
            //服务端接受来自客户端的轮询
            int n = recvfrom(server_fd, (char*)&recvMsg, 128, 0, (struct sockaddr *)&client_addr, &client_len);
            event = recvMsg.event;
            time = recvMsg.time;
            if (event == NEW_CLIENT) {
                welcomeClient();
                Nothing();
            } else if (event == FINISHED){
                cout << "客户端: "
                        << inet_ntoa(client_addr.sin_addr) << ":"
                        << client_addr.sin_port << "完成任务!" << endl;
                fputs(recvMsg.data, stdout);
                fputs("\n", stdout);
                printf("================================\n");
                Nothing();
            } else if (event == SENDFILE){
                cout << "客户端: "
                        << inet_ntoa(client_addr.sin_addr) << ":"
                        << client_addr.sin_port << "完成任务!" << endl;
                recvFile();
            } else if (event == RECVFILE){
                cout << "客户端: "
                        << inet_ntoa(client_addr.sin_addr) << ":"
                        << client_addr.sin_port << "准备接收文件!" << endl;
                sendMsg.event = SENDFILE;
                char target[256];
                bzero(target, sizeof(target));
                memcpy(target, recvMsg.data + 7, strlen(recvMsg.data) - 7);
                sendFile(target);
            } else {
                if (fd == client_addr.sin_port) {
                    sendMsg.event = EXCUTE_CMD;
                    strcpy(sendMsg.data, cmd);
                    sendMsg.time = 200;
                    sendto(server_fd, (char*)&sendMsg, sizeof(Msg), 0, (struct sockaddr *) &client_addr, client_len);
                    fd = -1;
                } else {
                    Nothing();
                }
            }
            count++;
            if (count == 10) break;
        }
    } 
    //关闭服务
    Close();
}