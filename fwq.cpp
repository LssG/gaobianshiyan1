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
	bool operator==(const socket_type var) const{
		return this->sockfd==var.sockfd;
	}
	bool operator==(const int var) const{
		return this->sockfd==var;
	}
};

// int roomnum[256];
int socketfd = -1;
list<socket_type> sockaddrs[MAXROOM];

void closeAll();

void sendmes(int fd,char* mes);

bool showsocks();

void sockaddrcon(int flag,socket_type addr);

void admin_check(char *buf);

void *admin_woeker(void *arg);
void *recv_worker(void *arg);

int  main(int argc,char* argv[]){

	char* my_ip = SERVER_IP;
	int new_fd = -1,admin_fd;
	socklen_t len;
	struct sockaddr_in their_addr,my_addr;
	unsigned int myport = SERVER_PORT,lisnum;
	pthread_t tid;

	if(argc == 3){
		my_ip = argv[1];
		myport = atoi(argv[2]);
	}
	else if(argc == 2){
		my_ip = argv[1];
	}
	else if(argc > 3){
		printf("error!\nInvalid number of arguments!\n");
		exit(0);
	}

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
	printf("wait for connect.\n");
	len = sizeof(struct sockaddr);

	pthread_create(&tid,NULL,admin_woeker,NULL);

	while((new_fd = accept(socketfd,(struct sockaddr *)&their_addr,&len)) != -1){
		
		socket_type tmp = {new_fd,their_addr};
		
		pthread_create(&tid,NULL,recv_worker,(void*)&tmp);

		//向地址链表中添加链接
		sockaddrcon(_ADD,tmp);
		cout<<"server:got connection from " << inet_ntoa(their_addr.sin_addr) << ":" << ntohs(their_addr.sin_port) << ", socket " << new_fd<<endl;
		
	}
	perror("accept");
	exit(EXIT_FAILURE);

	return 0;
}

void *admin_woeker(void *arg){
	pthread_detach(pthread_self());
	char buf[MAXSIZE+1];
	while(1){
		cout << "input \"send\" to send message,input \"exit\" to exit." <<endl;
		bzero(buf,MAXSIZE+1);
		cin.get(buf,MAXSIZE);
		cin.ignore(INT_MAX,'\n');
		if(!strncasecmp(buf,"send",4)){
			if(showsocks()){
				int i;
				printf("input fd:\n");
				bzero(buf,MAXSIZE+1);
				cin.get(buf,MAXSIZE);
				cin.ignore(MAXSIZE,'\n');
				admin_check(buf);
				i = atoi(buf);
				printf("input mes:\n");
				bzero(buf,MAXSIZE+1);
				cin.get(buf,MAXSIZE);
				cin.ignore(MAXSIZE,'\n');
				admin_check(buf);
				sendmes(i,buf);
			}
		}else if(!strncasecmp(buf,"exit",4)){
			closeAll();
			exit(0);
		}
	}
}

void *recv_worker(void *arg){
	pthread_detach(pthread_self());
	socket_type tmp = *((socket_type*)arg);
	struct sockaddr_in their_addr = tmp.addr;
	int new_fd = tmp.sockfd;
	int len;
	char buf[MAXSIZE+1];
	while(1){
		bzero(buf,MAXSIZE+1);
		len = recv(new_fd,buf,MAXSIZE,0);
		if(len > 0){
			if(!strncasecmp(buf,"exit",4)){
				printf("client %s:%d will close the connect!\n",inet_ntoa(their_addr.sin_addr),ntohs(their_addr.sin_port));
				send(new_fd,buf,strlen(buf),0);
				sockaddrcon(_REM,tmp);
				break;
			}
			printf("\nmessage recv successful :from %s:%d,'%s' %dBytes recv.\n",inet_ntoa(their_addr.sin_addr),ntohs(their_addr.sin_port),buf,len);
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
	list<socket_type>::iterator ite = sockaddrs.begin();
	for(; ite != sockaddrs.end(); ++ite){
		sendmes((*ite).sockfd,"exit");
		shutdown((*ite).sockfd,SHUT_RDWR);
		close((*ite).sockfd);
	}
	shutdown(socketfd,SHUT_RDWR);
	close(socketfd);
}

void sendmes(int fd,char* mes){
	//cout << "test:sendmes-mes=\"" << mes <<"\""<<endl;
	int len = write(fd,mes,strlen(mes));
	if(len < 0){
		perror("sendmes");
		closeAll();
		exit(errno);
	}else if(len>0){
		printf("message successful!\n");
	}else{
		//cout << "test:len = 0" <<endl;
	}
}

bool showsocks(){
	bool re = !(sockaddrs.empty());
	//cout << "test:shutdown-re=" << re <<endl;
	if(re){
		list<socket_type>::iterator ite = sockaddrs.begin();
		cout <<"\tfd\t\taddr" << endl;
		for(;ite != sockaddrs.end(); ++ite){
			cout << "\t" << (*ite).sockfd << "\t\t" << inet_ntoa((*ite).addr.sin_addr) << ":" << ntohs((*ite).addr.sin_port) <<endl;
		}
	}else{
		printf("no connection!\n");
	}
	return re;
}

void sockaddrcon(int flag,socket_type addr){
	switch(flag){
		case _ADD:
		sockaddrs.push_back(addr);
		//cout << "test:sockaddrcon-_ADD-pid()="<<getpid()<<"sockaddrs.size()=" << sockaddrs.size()<<",sockaddrs.empty()="<<sockaddrs.empty()<<endl;
		break;
		case _REM:
		shutdown(addr.sockfd,SHUT_RDWR);
		close(addr.sockfd);
		sockaddrs.remove(addr);
		//cout << "test:sockaddrcon-_REM-pid()="<<getpid()<<"sockaddrs.size()=" << sockaddrs.size()<<",sockaddrs.empty()="<<sockaddrs.empty()<<endl;
		break;
	}
	//cout << "test:sockaddrcon-pid()="<<getpid()<<"sockaddrs.size()=" << sockaddrs.size()<<",sockaddrs.empty()="<<sockaddrs.empty()<<endl;
}

void admin_check(char *buf){
	if(!strncasecmp(buf,"exit",4)){
		closeAll();
		exit(0);
	}
}

