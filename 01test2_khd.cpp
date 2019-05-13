#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>
using namespace std;

#define MAXSIZE 1024

#define SERVER_PORT	7171
// #define SERVER_IP	"101.132.162.118"
#define SERVER_IP	"127.0.0.1"
int sockfd;

void closeAll(){
	shutdown(sockfd,SHUT_RDWR);
	close(sockfd);
	exit(0);
}

void sig_int(int sig){
	closeAll();
}

int main(int argc, char* argv[]){

	signal(SIGINT,sig_int);

	int len;
	char *dest_ip = SERVER_IP;
	short dest_port = SERVER_PORT;
	socklen_t soclen;
	char isrun = 1;
	char buf[MAXSIZE+1];


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

	pid_t pid;
	pid = fork();
	if(pid < 0){
		perror("fork");
		exit(EXIT_FAILURE);
	}
	else if(pid == 0){			//子进程发送数据
		while(isrun){
			bzero(&buf,MAXSIZE+1);
			printf("pls send message to send(input \"exit\" to exit):");
			fgets(buf,MAXSIZE,stdin);
			if(!strncasecmp(buf,"exit",4)){
				printf("i will close the connect!\n");
				isrun = 0;
			}
			len = write(sockfd,buf,strlen(buf) - 1);
			if(len < 0){
				printf("send failure , errno code is %d.\n",errno);
				isrun = 0;
			}
		}
	}
	else{						//父进程接收数据
		while(isrun){
			bzero(&buf,MAXSIZE+1);
			len = recv(sockfd,buf,MAXSIZE,0);
			if(len < 0){
				printf("recv failure , errno code is %d.\n",errno);
				break;
			}else if(len == 0){
				printf("\nserver closed!\n");
				break;
			}
			if(!strncasecmp(buf,"exit",4)){
				printf("server will close the connect!\n");
				break;
			}
			
			printf("\nmessage recv successful :'%s' %dBytes recv.\n",buf,len);
		}
	}
	kill(0,SIGINT);
	return 0;
}
