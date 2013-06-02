#define _GNU_SOURCE 
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <signal.h>
#include <wait.h>
#include <netdb.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#define ERR(source) (perror(source),\
		     fprintf(stderr,"%s:%d\n",__FILE__,__LINE__),\
		     exit(EXIT_FAILURE))
#define HERR(source) (fprintf(stderr,"%s(%d) at %s:%d\n",source,h_errno,__FILE__,__LINE__),\
		     exit(EXIT_FAILURE))

#define BACKLOG 3
#define MSGCOUNT 1000

volatile sig_atomic_t last_signal=0 ;

void sigchld_handler(int);

void sigint_handler(int sig) {
	        last_signal=sig;
}       

int sethandler( void (*f)(int), int sigNo) {
	struct sigaction act;
	memset(&act, 0, sizeof(struct sigaction));
	act.sa_handler = f;
	if (-1==sigaction(sigNo, &act, NULL))
		return -1;
	return 0;
}

void sigchld_handler(int sig) {
	pid_t pid;
	for(;;){
		pid=waitpid(0, NULL, WNOHANG);
		if(0==pid) return;
		if(0>=pid) {
			if(ECHILD==errno) return;
			ERR("waitpid:");
		}
	}
}

ssize_t bulk_read(int fd, char *buf, size_t count){
	int c;
	size_t len=0;
	do{
		c=TEMP_FAILURE_RETRY(read(fd,buf,count));
		if(c<0) return c;
		if(0==c) return len;
		buf+=c;
		len+=c;
		count-=c;
	}while(count>0);
	return len ;
}

int make_socket(int domain, int type){
	int sock;
	sock = socket(domain,type,0);
	if(sock < 0) ERR("socket");
	return sock;
}

int bind_inet_socket(uint16_t port,int type){
	struct sockaddr_in addr;
	int socketfd,t=1;
	socketfd = make_socket(PF_INET,type);
	memset(&addr, 0, sizeof(struct sockaddr_in));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	if (setsockopt(socketfd, SOL_SOCKET, SO_REUSEADDR,&t, sizeof(t))) ERR("setsockopt");
	if(bind(socketfd,(struct sockaddr*) &addr,sizeof(addr)) < 0)
		if(SOCK_STREAM==type)
			if(errno == EADDRINUSE){
				printf("Address in use\n");
				if(TEMP_FAILURE_RETRY(close(socketfd))<0)ERR("close");
				return -1;
			}
	if(SOCK_STREAM==type){
		if((listen(socketfd, BACKLOG)) < 0) ERR("listen");
	}
	return socketfd;
}

struct sockaddr_in make_address(char *address, uint16_t port){
	struct sockaddr_in addr;
	struct hostent *hostinfo;
	addr.sin_family = AF_INET;
	addr.sin_port = htons (port);
	hostinfo = gethostbyname(address);
	if(hostinfo == NULL)HERR("gethostbyname");
	addr.sin_addr = *(struct in_addr*) hostinfo->h_addr;
	return addr;
}

void child_work(int fds, sigset_t *oldmask) {
	int readC, i, result=-2;
	last_signal = 0;
	
	fd_set base_rfds, rfds ;
	FD_ZERO(&base_rfds);
	FD_SET(fds, &base_rfds);
	
	while(last_signal == 0){
		rfds=base_rfds;
		if((result = pselect(fds+1,&rfds,NULL,NULL,NULL,oldmask))>0){
			char buf[MSGCOUNT];
			if(TEMP_FAILURE_RETRY(readC = read(fds,buf,MSGCOUNT)) < 0) ERR("read");
			for(i=0; i<readC; i++)
			{
				printf("%c",buf[i]);
				fflush(stdout);
			}
			printf("\n");
			if(readC == 0){
				printf("Client disconnected\n");
				if(fds>0&&TEMP_FAILURE_RETRY(close(fds))<0)ERR("close");
				return;
			}
		}
		else if(result == 0)
			break;
	}
	printf("SIGINT received or EOF\n");
}


void create_child(int fds, sigset_t *oldmask) {
	switch (fork()) {
		case 0:
			child_work(fds, oldmask);
			exit(EXIT_SUCCESS);
		case -1: ERR("Fork:");
	}
}

void closeConnections(int fdU, int fdT){
	kill(0, SIGINT);
	if(TEMP_FAILURE_RETRY(close(fdT)<0)) ERR("close");
	if(TEMP_FAILURE_RETRY(close(fdU)<0)) ERR("close");
}

void doServer(int fdU){
	struct sockaddr_in addr;
	socklen_t size=sizeof(addr);
	int16_t p;
	int fdT=0, fdmax=fdU, flags, nfd, p_old;
	
	sigset_t mask, oldmask;
	sigemptyset (&mask);
	sigaddset (&mask, SIGINT);
	sigprocmask (SIG_BLOCK, &mask, &oldmask);
	
	fd_set base_rfds, rfds ;
	FD_ZERO(&base_rfds);
	FD_SET(fdU, &base_rfds);
	
	while(last_signal == 0){
		rfds=base_rfds;
		if(pselect(fdmax+1,&rfds,NULL,NULL,NULL,&oldmask)>0){
			if(FD_ISSET(fdU, &rfds))
			{
				p_old = p;
				if(TEMP_FAILURE_RETRY(recvfrom(fdU,(char*)(&p),sizeof(int16_t),0,&addr,&size)<0)) ERR("recvfrom:");
				p=ntohs(p);
				if(p == p_old)
					continue;
				printf("New TCP port value %d received\n", p);
				if(fdT > 0){
					FD_CLR(fdT, &base_rfds);
					if(TEMP_FAILURE_RETRY(close(fdT)<0)) ERR("close");
					if(sethandler(SIG_IGN,SIGTERM)) ERR("Seting SIGTERM:");
					kill(0,SIGTERM);
					if(sethandler(SIG_DFL,SIGTERM)) ERR("Seting SIGTERM:");
				}
		
				if((fdT = bind_inet_socket(p, SOCK_STREAM)) < 0)
					continue;	
				flags = fcntl(fdT, F_GETFL) | O_NONBLOCK;
				fcntl(fdT, F_SETFL, flags);
				fdmax = fdT > fdU ? fdT : fdU;
				FD_SET(fdT, &base_rfds);
			}
			if(FD_ISSET(fdT, &rfds))
			{
				if((nfd=TEMP_FAILURE_RETRY(accept(fdT,NULL,NULL)))<0) ERR("accept");
				create_child(nfd, &oldmask);	
				if(TEMP_FAILURE_RETRY(close(nfd))<0)ERR("close");
			}
		}
	}
	closeConnections(fdU, fdT);
}

int main(int argc, char** argv) {
	int fd;
	if(sethandler(SIG_IGN,SIGPIPE)) ERR("Seting SIGPIPE:");
	if(sethandler(sigint_handler,SIGINT)) ERR("Seting SIGINT:");
	if(sethandler(sigchld_handler,SIGCHLD)) ERR("Setting SIGCHLD:");
	fd = bind_inet_socket(2000,SOCK_DGRAM);
	doServer(fd);
			
	while(TEMP_FAILURE_RETRY(wait(NULL)) >= 0); 
	printf("Server terminated\n");
	return EXIT_SUCCESS;
}
