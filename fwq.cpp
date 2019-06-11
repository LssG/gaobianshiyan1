#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <list>
#include <unistd.h>
#include <signal.h>
#include <iostream>
#include <climits>
#include <pthread.h>
#include <sys/time.h>
using namespace std;

#define MAXSIZE 1024

// #define SERVER_IP "172.19.79.179"
#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 7171
#define MAXROOM 256

#define _ADD	1
#define _REM	2

struct socket_type
{
	int sockfd;
	sockaddr_in addr;
	int room;
	bool operator==(const socket_type var) const{
		return this->sockfd==var.sockfd;
	}
	bool operator==(const int var) const{
		return this->sockfd==var;
	}
	bool operator!=(const socket_type var) const{
		return this->sockfd!=var.sockfd;
	}
};

// int roomnum[256];
int socketfd = -1;
list<socket_type> sockaddrs[MAXROOM];

void closeAll();

void sendmes(int fd,char* mes,size_t mlen);

void sendmestoroom(socket_type tmp,char *mes,size_t len);
void sendmestoroom(int room,char *mes,size_t len);

void sockaddrcon(int room,int flag,socket_type addr);

void *khd_worker(void *arg);

int  main(int argc,char* argv[]){

	char* my_ip = SERVER_IP;
	int new_fd = -1,admin_fd;
	socklen_t len;
	struct sockaddr_in their_addr,my_addr;
	unsigned int myport = SERVER_PORT,lisnum;
	pthread_t tid;

	if((socketfd = socket(AF_INET,SOCK_STREAM,0)) == -1){
		perror("socket");
		closeAll();
		exit(EXIT_FAILURE);
	}

	bzero(&my_addr,sizeof(my_addr));
	my_addr.sin_port = htons(myport);
	my_addr.sin_family = AF_INET;
	my_addr.sin_addr.s_addr = inet_addr(my_ip);

	if(bind(socketfd, (struct sockaddr *)&my_addr,sizeof(my_addr)) == -1){
		perror("bind");
		closeAll();
		exit(EXIT_FAILURE);
	}
	if(listen(socketfd,lisnum)==-1){
		perror("listen");
		closeAll();
		exit(EXIT_FAILURE);
	}
	// printf("wait for connect.\n");
	// pthread_create(&tid,NULL,admin_woeker,NULL);
	len = sizeof(struct sockaddr);

	while((new_fd = accept(socketfd,(struct sockaddr *)&their_addr,&len)) != -1){
		
		socket_type tmp = {new_fd,their_addr,-1};
		
		pthread_create(&tid,NULL,khd_worker,(void*)&tmp);

		//向地址链表中添加链接
		// sockaddrcon(_ADD,tmp);
		// cout<<"server:got connection from " << inet_ntoa(their_addr.sin_addr) << ":" << ntohs(their_addr.sin_port) << ", socket " << new_fd<<endl;
		
	}
	perror("accept");
	exit(EXIT_FAILURE);

	return 0;
}

void *khd_worker(void *arg){
	pthread_detach(pthread_self());

	char buf[MAXSIZE+1];
	socket_type tmp = *((socket_type*)arg);
	struct sockaddr_in their_addr = tmp.addr;
	int new_fd = tmp.sockfd;
	int len;

	bzero(buf,MAXSIZE+1);
	len = recv(new_fd,buf,MAXSIZE,0);
	tmp.room = (int)buf[0];
	//向地址链表中添加链接
	sockaddrcon(tmp.room,_ADD,tmp);
	buf[0] = (char)(sockaddrs[tmp.room].size());
	sendmestoroom(tmp.room,buf,1);
	while(1){
		bzero(buf,MAXSIZE+1);
		len = recv(new_fd,buf,MAXSIZE,0);
		if(len > 0){
			if(!strncasecmp(buf,"exit",4)){
				printf("client %s:%d will close the connect!\n",inet_ntoa(their_addr.sin_addr),ntohs(their_addr.sin_port));
				sockaddrcon(tmp.room,_REM,tmp);
				break;
			}
			sendmestoroom(tmp,buf,strlen(buf));
		}else if(len < 0){
			printf("recv failure , errno code is %d.\n",errno);
			break;
		}else{
			printf("the other one close quit\n");
			break;
		}
	}
}

void closeAll(){
	int i ;
	for(i = 0; i < MAXROOM; ++i){
		list<socket_type>::iterator ite = sockaddrs[i].begin();
		for(; ite != sockaddrs[i].end(); ++ite){
			sendmes((*ite).sockfd,"exit",4);
			shutdown((*ite).sockfd,SHUT_RDWR);
			close((*ite).sockfd);
		}
	}
	shutdown(socketfd,SHUT_RDWR);
	close(socketfd);
}

void sendmes(int fd,char* mes,size_t mlen){
	//cout << "test:sendmes-mes=\"" << mes <<"\""<<endl;
	int len = write(fd,mes,mlen);
	if(len < 0){
		perror("sendmes");
		// closeAll();
		exit(errno);
	}else if(len>0){
		printf("message successful!\n");
	}else{
		//cout << "test:len = 0" <<endl;
	}
}

void sockaddrcon(int room,int flag,socket_type addr){
	switch(flag){
		case _ADD:
		sockaddrs[room].push_back(addr);
		//cout << "test:sockaddrcon-_ADD-pid()="<<getpid()<<"sockaddrs.size()=" << sockaddrs.size()<<",sockaddrs.empty()="<<sockaddrs.empty()<<endl;
		break;
		case _REM:
		shutdown(addr.sockfd,SHUT_RDWR);
		close(addr.sockfd);
		sockaddrs[room].remove(addr);
		//cout << "test:sockaddrcon-_REM-pid()="<<getpid()<<"sockaddrs.size()=" << sockaddrs.size()<<",sockaddrs.empty()="<<sockaddrs.empty()<<endl;
		break;
	}
	//cout << "test:sockaddrcon-pid()="<<getpid()<<"sockaddrs.size()=" << sockaddrs.size()<<",sockaddrs.empty()="<<sockaddrs.empty()<<endl;
}

int mySleep(unsigned int sleepSecond)
{
	struct timeval t_timeval;
	t_timeval.tv_sec = 0;
	t_timeval.tv_usec = sleepSecond;
	select( 0, NULL, NULL, NULL, &t_timeval );
	return 0;
}

void sendmestoroom(socket_type tmp,char *mes,size_t len){
	list<socket_type>::iterator ite = sockaddrs[tmp.room].begin();
	for(;ite != sockaddrs[tmp.room].end(); ++ite){
		if(tmp!=(*ite)){
			sendmes((*ite).sockfd,mes,len);
		}
	}
}

void sendmestoroom(int room,char *mes,size_t len){
	list<socket_type>::iterator ite = sockaddrs[room].begin();
	for(;ite != sockaddrs[room].end(); ++ite){
		sendmes((*ite).sockfd,mes,len);
	}
}