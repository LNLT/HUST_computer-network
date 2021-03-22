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
	//��ʼ��winsock
	isok=WSAStartup(0x0202, &wsdata);//ȷ��socket�汾��Ϣ2.2��makeword��һ����
	error(isok, WSAEINVAL, "��ʼ��Winsockʧ��!\n");
	printf("LTT��������\n");
	//��������socket
	SOCKET ser_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);//����������Э����(�˴���ipv4�����������ͣ��˴������䣩������Э�飨TCP��
	error(ser_socket, INVALID_SOCKET, "����socketʧ�ܣ�\n");
	
	//����socket
	int port = 0;
	char inaddr[20] = "";
	printf("������˿ںţ�\n");
	scanf("%d", &port);
	printf("�����������ַ��\n");
	scanf("%s", inaddr);
	char filename[48] = "";
	printf("��������Ŀ¼��\n");
	scanf("%s", filename);
	
	
	struct sockaddr_in seraddr;
	seraddr.sin_family = AF_INET;//��socket��Э��һ��Ϊipv4
	seraddr.sin_port = htons(port);//���˿ں�80����������Զ����Ĵ洢��ʽ�������ˣ�����С��
	seraddr.sin_addr.s_addr = inet_addr(inaddr);//������ַ


	isok = bind(ser_socket, &seraddr, sizeof(seraddr));
	error(isok, SOCKET_ERROR, "������ʧ�ܣ�\n");
	
	//���õȴ�����״̬
	isok = listen(ser_socket, 5);
	error(isok, SOCKET_ERROR, "���õȴ�����״̬ʧ�ܣ�\n");
	
	
	struct sockaddr_in claddr;
	int cllen = sizeof(claddr);
	while (1)
	{
		//�ȴ����Ӳ����ɻỰsocket
		SOCKET cli_socket = accept(ser_socket, &claddr, &cllen);//˭���ͻ��ˣ�������
		error(cli_socket, INVALID_SOCKET, "����ʧ�ܣ�\n");
		

		printf("�ͻ��˵�IP�Ͷ˿ں��ǣ�%s:%u\n", inet_ntoa(claddr.sin_addr),htons(claddr.sin_port));
		//���лỰ
		char recvdata[1024] = "";
		isok = recv(cli_socket, recvdata, 1024, 0);
		error(isok, SOCKET_ERROR, "��������ʧ�ܣ�\n");
		printf("%s�����յ�%d�ֽ�����\n", recvdata, strlen(recvdata));

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
		printf("�ļ�����%s\n", name);
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
	char* extension = file_type_addr(arguments);//�����ļ�����
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

	char* head = "HTTP/1.1 200 OK\r\n";//������Ӧ����ͷ����������
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

char* file_type_addr(char* arg)//��λ�ļ�����׺λ��
{
	char* temp;
	if ((temp = strrchr(arg, '.')) != NULL)
	{
		return temp + 1;
	}
	return "";
}

void send_file(SOCKET s, char* filename)//�����ļ����ͻ���
{
	int ok;
	FILE* pfile = fopen(filename, "rb");
	if (pfile == NULL)
	{
		printf("���ļ�ʧ�ܣ�\n");
		return 0;
	}
	fseek(pfile, 0L, SEEK_END);
	int flen = ftell(pfile);
	char *p = (char*)malloc(flen + 1);
	fseek(pfile, 0L, SEEK_SET);
	fread(p, flen, 1, pfile);
	send(s, p, flen, 0);
}
