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
#include <netdb.h>
#include <time.h>
#define ERR(source) (perror(source),\
		     fprintf(stderr,"%s:%d\n",__FILE__,__LINE__),\
		     exit(EXIT_FAILURE))
#define HERR(source) (fprintf(stderr,"%s(%d) at %s:%d\n",source,h_errno,__FILE__,__LINE__),\
		     exit(EXIT_FAILURE))	     
#define MSGCOUNT 1000


int sethandler( void (*f)(int), int sigNo) {
	struct sigaction act;
	memset(&act, 0, sizeof(struct sigaction));
	act.sa_handler = f;
	if (-1==sigaction(sigNo, &act, NULL))
		return -1;
	return 0;
}

int make_socket(void){
	int sock;
	sock = socket(PF_INET,SOCK_STREAM,0);
	if(sock < 0) ERR("socket");
	return sock;
}

int make_socketU(void){
	int sock;
	sock = socket(PF_INET,SOCK_DGRAM,0);
	if(sock < 0) ERR("socket");
	return sock;
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

int connect_socket(char *name, uint16_t port){
	struct sockaddr_in addr;
	int socketfd;
	socketfd = make_socket();
	addr=make_address(name,port);
	if(connect(socketfd,(struct sockaddr*) &addr,sizeof(struct sockaddr_in)) < 0){
		if(errno!=EINTR) ERR("connect");
		else { 
			fd_set wfds ;
			int status;
			socklen_t size = sizeof(int);
			FD_ZERO(&wfds);
			FD_SET(socketfd, &wfds);
			if(TEMP_FAILURE_RETRY(select(socketfd+1,NULL,&wfds,NULL,NULL))<0) ERR("select");
			if(getsockopt(socketfd,SOL_SOCKET,SO_ERROR,&status,&size)<0) ERR("getsockopt");
			if(0!=status) ERR("connect");
		}
	}
	return socketfd;
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

ssize_t bulk_write(int fd, char *buf, size_t count){
	int c;
	size_t len=0;
	do{
		c=TEMP_FAILURE_RETRY(write(fd,buf,count));
		if(c<0) return c;
		buf+=c;
		len+=c;
		count-=c;
	}while(count>0);
	return len ;
}

void usage(char * name){
	fprintf(stderr,"USAGE: %s domain port\n",name);
}

void doClient(int16_t port, char* domain){
	int fdT;
	char buf[MSGCOUNT];
	
	fdT=connect_socket(domain, port);
	memset((void*)buf, 0, MSGCOUNT);
	printf("Client is ready to write\n");
	while(fgets(buf,MSGCOUNT,stdin)){
		if(strlen(buf)) buf[strlen(buf)-1]='\0';
		if(bulk_write(fdT,buf,strlen(buf))<0){
			if(errno == EPIPE){
				printf("Server is closed or is not listening to the socket on port %d\n", port);
				break;
			}
			ERR("send");
		}
	}
	if(TEMP_FAILURE_RETRY(close(fdT))<0)ERR("close");
}

int main(int argc, char** argv) {
	int fdU;
	struct sockaddr_in addr;
	
	int16_t p = (int16_t)atoi(argv[2]);
	if(argc!=3) {
		usage(argv[0]);
		return EXIT_FAILURE;
	}
	
	if(sethandler(SIG_IGN,SIGPIPE)) ERR("Seting SIGPIPE:");
	fdU = make_socketU();
	addr=make_address(argv[1],2000);
	p = htons(p);

	if(TEMP_FAILURE_RETRY(sendto(fdU,(char *)&p,sizeof(int16_t),0,&addr,sizeof(addr)))<0)
		ERR("sendto:");
	
	printf("Sleeping...\n");
	struct timespec t = {1,0};
	while(nanosleep(&t,&t))
		if(EINTR!=errno) ERR("nanosleep:");
		
	doClient(ntohs(p), argv[1]);

	if(TEMP_FAILURE_RETRY(close(fdU))<0)ERR("close");
	return EXIT_SUCCESS;
}
