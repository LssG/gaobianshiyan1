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
using namespace std;

#define MAXSIZE 1024

// #define SERVER_IP "172.19.79.179"
#define SERVER_IP "127.0.0.1"

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

int socketfd = -1;
list<socket_type> sockaddrs;
char buf[MAXSIZE+1];
bool isrun = true;
pid_t main_pid;
int p[2];

void closeAll(){
	list<socket_type>::iterator ite = sockaddrs.begin();
	for(; ite != sockaddrs.end(); ++ite){
		shutdown((*ite).sockfd,SHUT_RDWR);
		close((*ite).sockfd);
	}
	shutdown(socketfd,SHUT_RDWR);
	close(socketfd);
	close(p[0]);
	close(p[1]);
	exit(0);
}

void sendmes(int fd,char* mes){
	// bzero(&buf,MAXSIZE+1);
	// memcpy(&buf,mes,strlen(mes));
	//cout << "test:sendmes-buf=\"" << buf <<"\""<<",mes=\"" << mes <<"\""<<endl;
	int len = write(fd,buf,strlen(buf));
	if(len < 0){
		perror("sendmes");
		closeAll();
		exit(errno);
	}else if(len>0){
		printf("message send successful!\n");
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

void sig_fun(int sig){
	closeAll();
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

void admin_end(){
	kill(0,SIGUSR2);
	exit(0);
}

void admin_check(){
	if(!strncasecmp(buf,"exit",4)){
		kill(0,SIGINT);
		closeAll();
		exit(0);
	}
}

void sig_admin(int sig){
	//fflush(stdin);
	//fflush(stdout);
	pid_t pid = fork();
	if(pid < 0){
		perror("fork");
		closeAll();
		exit(EXIT_FAILURE);
	}else if(pid == 0){
		printf("send message\n");
		if(showsocks()){
			int i;
			printf("input fd:\n");
			//fflush(stdout);
			// //fflush(stdin);
			bzero(buf,MAXSIZE+1);
			cin.get(buf,MAXSIZE);
			cin.ignore(MAXSIZE,'\n');
			admin_check();
			i = atoi(buf);
			printf("input mes:\n");
			//fflush(stdin);
			//fflush(stdout);
			bzero(buf,MAXSIZE+1);
			cin.get(buf,MAXSIZE);
			cin.ignore(MAXSIZE,'\n');
			admin_check();
			// buf[strlen(buf)-1] = 0;
			//fflush(stdin);
			//cout << "test:i="<<i<<",buf=\""<<buf<<"\",buflen="<<strlen(buf)<<endl;
			sendmes(i,buf);
		}
		admin_end();
	}else
		return;
}

void sig_usr2(int sig){
	isrun = false;
}

void sig_winch(int sig){
	char buf[MAXSIZE];
	read(p[0],buf,MAXSIZE);
	int fd = atoi(buf);
	list<socket_type>::iterator ite = sockaddrs.begin();
	for(;ite != sockaddrs.end(); ite++){
		if(*(ite) == fd){
			sockaddrs.erase(ite);
			return;
		}
	}
}


int  main(int argn,char* argv[]){
	main_pid = getpid();

	signal(SIGINT,sig_fun);
	signal(SIGUSR1,sig_admin);
	signal(SIGUSR2,sig_usr2);
	signal(SIGWINCH,sig_winch);

	char* my_ip = SERVER_IP;
	pid_t pid;
	int new_fd = -1,admin_fd;
	socklen_t len;
	struct sockaddr_in my_addr,their_addr,admin_addr;
	unsigned int myport = 7171,lisnum;

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

	pid = fork();
	if(pid == -1){
		perror("fork");
		closeAll();
		exit(EXIT_FAILURE);
	}else if(pid == 0){
		while(1){
			cout << "input \"send\" to send message,input \"exit\" to exit." <<endl;
			bzero(buf,MAXSIZE+1);
			//cout << "test:isrun=" << isrun<<endl;
			cin.get(buf,MAXSIZE);
			cin.ignore(INT_MAX,'\n');
			if(!strncasecmp(buf,"send",4)){
				isrun = true;
				//cout << "test:kill"<<endl;
				kill(main_pid,SIGUSR1);
				//cout << "test:kill down"<<endl;
				while(isrun) sleep(1);
			}else if(!strncasecmp(buf,"exit",4)){
				kill(0,SIGINT);
				closeAll();
				exit(0);
			}
		}
	}

	pipe(p);
	while((new_fd = accept(socketfd,(struct sockaddr *)&their_addr,&len)) != -1){
		
		socket_type tmp = {new_fd,their_addr};
		//fflush(stdin);
		//fflush(stdout);
		pid = fork();
		if(pid < 0){
			perror("fork");
			closeAll();
			exit(EXIT_FAILURE);
		}
		else if(pid == 0){				//子进程收发数据
			while(1){
				if(new_fd==-1){
					// printf("wulianjie\n");
					exit(0);
				}
				// close(p[0]);

				//接收数据
				bzero(buf,MAXSIZE+1);
				len = recv(new_fd,buf,MAXSIZE,0);
				if(len > 0){
					if(!strncasecmp(buf,"exit",4)){
						printf("client %s:%d will close the connect!\n",inet_ntoa(their_addr.sin_addr),ntohs(their_addr.sin_port));
						send(new_fd,buf,strlen(buf),0);
						sockaddrcon(_REM,tmp);
						char itoa_tmp[MAXSIZE];
						sprintf(itoa_tmp,"%d",new_fd);
						write(p[1],itoa_tmp,MAXSIZE);
						kill(main_pid,SIGWINCH);
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
			exit(0);
		}else{
			//向地址链表中添加链接
			sockaddrcon(_ADD,tmp);
			//fflush(stdout);
			cout<<"server:got connection from " << inet_ntoa(their_addr.sin_addr) << ":" << ntohs(their_addr.sin_port) << ", socket " << new_fd<<endl;
		}
	}
	perror("accept");
	exit(EXIT_FAILURE);

	return 0;
}
