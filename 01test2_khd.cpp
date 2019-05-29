#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <pthread.h>
using namespace std;

#define MAXSIZE 1024

#define SERVER_PORT	7171
// #define SERVER_IP	"101.132.162.118"
#define SERVER_IP	"127.0.0.1"
int sockfd;
char isrun = 1;

int len;
char *dest_ip = SERVER_IP;
short dest_port = SERVER_PORT;
socklen_t soclen;
char fsbuf[MAXSIZE+1],jsbuf[MAXSIZE+1];

void *fasong(void *arg){
	while(isrun){
		bzero(&fsbuf,MAXSIZE+1);
		printf("pls send message to send(input \"exit\" to exit):");
		fgets(fsbuf,MAXSIZE,stdin);
		if(!strncasecmp(fsbuf,"exit",4)){
			printf("i will close the connect!\n");
			isrun = 0;
		}
		len = write(sockfd,fsbuf,strlen(fsbuf) - 1);
		if(len < 0){
			printf("send failure , errno code is %d.\n",errno);
			isrun = 0;
		}
	}
}

int main(int argc, char* argv[]){


	if(argc == 3){
		dest_ip = argv[1];
		dest_port = atoi(argv[2]);
	}
	else if(argc == 2){
		dest_ip = argv[1];
	}
	else if(argc > 3){
		printf("error!\nInvalid number of arguments!\n");
		exit(0);
	}

	struct sockaddr_in dest;
	bzero(&dest,sizeof(dest));
	dest.sin_family = AF_INET;
	dest.sin_port = htons(dest_port);
	dest.sin_addr.s_addr = inet_addr(dest_ip);

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

	pthread_t tid;
	pthread_create(&tid,NULL,fasong,NULL);

	// printf("test:aaa");

	while(isrun){				//接收数据进程
		bzero(&jsbuf,MAXSIZE+1);
		len = recv(sockfd,jsbuf,MAXSIZE,0);
		if(len < 0){
			printf("recv failure , errno code is %d.\n",errno);
			isrun = 0;
			break;
		}else if(len == 0){
			printf("\nserver closed!Pause the \"Enter\" to exit!\n");
			isrun = 0;
			break;
		}
		if(!strncasecmp(jsbuf,"exit",4)){
			// printf("server will close the connect!\n");
			isrun = 0;
			break;
		}
		printf("\nmessage recv successful :'%s' %dBytes recv.\n",jsbuf,len);
	}

	// printf("test:exit\n");
	pthread_join(tid,NULL);
	// printf("test:%d exit\n",tid);

	return 0;
}
