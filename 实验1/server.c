#include<stdio.h>
#include<Winsock2.h>
#pragma comment(lib,"ws2_32.lib")

void send_file(SOCKET s, char* filename);
int error(int backdata, int errordata, char* printword);
void send_head(char* arguments, SOCKET s, char* filename);
char* file_type_addr(char* arg);

int error(int backdata, int errordata, char* printword)
{
	if (backdata == errordata)
	{
		perror(printword);
		WSAGetLastError();
		getchar();
		return -1;
	}
	return 0;
}
int main()
{
	int isok;
	WSADATA wsdata;
	//初始化winsock
	isok=WSAStartup(0x0202, &wsdata);//确定socket版本信息2.2，makeword做一个字
	error(isok, WSAEINVAL, "初始化Winsock失败!\n");
	printf("LTT服务器：\n");
	//创建监听socket
	SOCKET ser_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);//三个参数：协议族(此处是ipv4），传输类型（此处流传输），传输协议（TCP）
	error(ser_socket, INVALID_SOCKET, "创建socket失败！\n");
	
	//监听socket
	int port = 0;
	char inaddr[20] = "";
	printf("请输入端口号：\n");
	scanf("%d", &port);
	printf("请输入监听地址：\n");
	scanf("%s", inaddr);
	char filename[48] = "";
	printf("请输入主目录：\n");
	scanf("%s", filename);
	
	
	struct sockaddr_in seraddr;
	seraddr.sin_family = AF_INET;//与socket的协议一样为ipv4
	seraddr.sin_port = htons(port);//将端口号80换成网络可以读懂的存储方式，网络大端，电脑小端
	seraddr.sin_addr.s_addr = inet_addr(inaddr);//监听地址


	isok = bind(ser_socket, &seraddr, sizeof(seraddr));
	error(isok, SOCKET_ERROR, "监听绑定失败！\n");
	
	//设置等待连接状态
	isok = listen(ser_socket, 5);
	error(isok, SOCKET_ERROR, "设置等待连接状态失败！\n");
	
	
	struct sockaddr_in claddr;
	int cllen = sizeof(claddr);
	while (1)
	{
		//等待连接并生成会话socket
		SOCKET cli_socket = accept(ser_socket, &claddr, &cllen);//谁（客户端）连进来
		error(cli_socket, INVALID_SOCKET, "连接失败！\n");
		

		printf("客户端的IP和端口号是：%s:%u\n", inet_ntoa(claddr.sin_addr),htons(claddr.sin_port));
		//进行会话
		char recvdata[1024] = "";
		isok = recv(cli_socket, recvdata, 1024, 0);
		error(isok, SOCKET_ERROR, "接受数据失败！\n");
		printf("%s共接收到%d字节数据\n", recvdata, strlen(recvdata));

		int i = 0,j=0;
		char name[15]=" ";
		while (recvdata[i] != '/')
			i++;
		while (recvdata[i + 1] != ' ')
		{
			name[j] = recvdata[i+1];
			i++;
			j++;
		}
		name[j] = '\0';
		printf("文件名：%s\n", name);
		char filename2[48] = "";
		strcpy(filename2, filename);
		strcat(filename2, name);
		printf("path:%s\n", filename2);
		send_head(filename2, cli_socket,filename2);
		send_file(cli_socket, filename2);	
		
		closesocket(cli_socket);
	}

	closesocket(ser_socket);
	WSACleanup();

	getchar();
	return 0;
}

void send_head(char* arguments, SOCKET s, char* filename)
{
	char* extension = file_type_addr(arguments);//解析文件类型
	char* content_type = "text/plain";
	char* body_length = "Content-Length: ";

	if (strcmp(extension, "html") == 0)
	{
		content_type = "text/html";
	}
	if (strcmp(extension, "gif") == 0)
	{
		content_type = "image/gif";
	}
	if (strcmp(extension, "jpg") == 0)
	{
		content_type = "image/jpg";
	}
	if (strcmp(extension, "png") == 0)
	{
		content_type = "image/png";
	}

	char* head = "HTTP/1.1 200 OK\r\n";//构造响应报文头部，并发送
	char* not_find = "HTTP/1.1 404 NOT FOUND\r\n";
	int len, len1;
	char temp_1[30] = "Content-type: ";
	len = strlen(head);
	len1 = strlen(not_find);
	FILE* pfile = fopen(filename, "rb");
	if (pfile == NULL)
	{
		send(s, not_find, len1, 0);
	}
	else if (send(s, head, len, 0) == -1)
	{
		printf("Sending error");
		return;
	}
	if (content_type)
	{
		strcat(temp_1, content_type);
		strcat(temp_1, "\r\n");
		len = strlen(temp_1);

		if (send(s, temp_1, len, 0) == -1)
		{
			printf("Sending error!");
			return;
		}
	}
	send(s, "\r\n", 2, 0);
}

char* file_type_addr(char* arg)//定位文件名后缀位置
{
	char* temp;
	if ((temp = strrchr(arg, '.')) != NULL)
	{
		return temp + 1;
	}
	return "";
}

void send_file(SOCKET s, char* filename)//发送文件给客户端
{
	int ok;
	FILE* pfile = fopen(filename, "rb");
	if (pfile == NULL)
	{
		printf("打开文件失败！\n");
		return 0;
	}
	fseek(pfile, 0L, SEEK_END);
	int flen = ftell(pfile);
	char *p = (char*)malloc(flen + 1);
	fseek(pfile, 0L, SEEK_SET);
	fread(p, flen, 1, pfile);
	send(s, p, flen, 0);
}
