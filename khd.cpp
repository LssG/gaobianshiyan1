#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <iostream>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <pthread.h>
#include <string>
#include <stdio.h>
using namespace std;

#define MAXSIZE 1024
#define NAMEMAXSIZE 10

#define SERVER_PORT	7171
// #define SERVER_IP	"101.132.162.118"
#define SERVER_IP	"127.0.0.1"
int sockfd;
char isrun = 1;

char* dest_ip = SERVER_IP;
short dest_port = SERVER_PORT;
socklen_t soclen;
char jsbuf[MAXSIZE];
string name;
unsigned char roomnum;
string fsbuf;

void *fasong(void *arg);
// string [] fenge(string str);


int main(int argc, char* argv[]){

	struct sockaddr_in dest;
	bzero(&dest,sizeof(dest));
	dest.sin_family = AF_INET;
	dest.sin_port = htons(dest_port);
	dest.sin_addr.s_addr = inet_addr(dest_ip);
	int len;


	if((sockfd = socket(AF_INET,SOCK_STREAM,0)) == -1){
		perror("socket");
		exit(EXIT_FAILURE);
	}
	if(connect(sockfd,(struct sockaddr*)&dest,sizeof(dest))==-1){	
		perror("connect");
		exit(EXIT_FAILURE);
	}
	printf("server connected\n");
	// printf("test:adsdsdasda");

	cout<< "请输入房间号(0~255)：";
	int tmp;
	cin>>tmp;
	cin.ignore(MAXSIZE,'\n');
	roomnum = (unsigned char)tmp;
	cout << "请输入用户名（3~10个字符，只能包括英文和数字）：";
	cin >> name;
	len = write(sockfd,&roomnum,1);
	if(len < 0){
		printf("send failure , errno code is %d.\n",errno);
		isrun = 0;
	}
	len = recv(sockfd,jsbuf,MAXSIZE,0);
	if(len <= 0){
		printf("recv failure , errno code is %d.\n",errno);
		exit(0);
	}
	char bijiaotmp = 1;
	if(!strncasecmp(jsbuf,&bijiaotmp,1)){
		cout << "Waiting for others..."<<endl;
		len = recv(sockfd,jsbuf,MAXSIZE,0);
		if(len <= 0){
			printf("recv failure , errno code is %d.\n",errno);
			exit(0);
		}
	}
	fsbuf = name+" is online!";
	len = write(sockfd,fsbuf.data(),fsbuf.length());

	pthread_t tid;
	pthread_create(&tid,NULL,fasong,NULL);

	// printf("test:aaa");

	while(isrun){				//接收数据进程
		char jsbuf[MAXSIZE+1];
		bzero(&jsbuf,MAXSIZE+1);
		len = recv(sockfd,jsbuf,MAXSIZE,0);
		if(len < 0){
			printf("recv failure , errno code is %d.\n",errno);
			isrun = 0;
			break;
		}else if(len == 0){
			// printf("\nserver closed!Pause the \"Enter\" to exit!\n");
			isrun = 0;
			break;
		}else if(len == 1){
			continue;
		}
		if(!strncasecmp(jsbuf,"exit",4)){
			// printf("server will close the connect!\n");
			isrun = 0;
			break;
		}
		cout<< jsbuf <<endl;
	}

	// printf("test:exit\n");
	pthread_join(tid,NULL);
	// printf("test:%d exit\n",tid);

	return 0;
}

void *fasong(void *arg){
	int len;
	char tmp[MAXSIZE];
	while(isrun){
		// printf("pls send message to send(input \"exit\" to exit):");
		// cin.get(tmp,MAXSIZE);
		// cin.ignore(MAXSIZE,'\n');
		// cin>>tmp;
		// cin.flush();
		cin.getline(tmp,MAXSIZE);
		fsbuf = name;
		fsbuf.append(":\t");
		fsbuf.append(tmp);
		// fsbuf+="\n";
		len = write(sockfd,fsbuf.data(),fsbuf.length());
		if(len < 0){
			printf("send failure , errno code is %d.\n",errno);
			isrun = 0;
		}
		if(!strncasecmp(tmp,"exit",4)){
			printf("i will close the connect!\n");
			len = write(sockfd,tmp,strlen(tmp));
			isrun = 0;
			if(len < 0){
				printf("send failure , errno code is %d.\n",errno);
				break;
			}
		}
	}
}

// string [] fenge(string str){
// 	unsigned int len;
// 	len = find_last_of('~',0);
// 	string strs[3];
// 	strs[2] = str.substr(len);

// }