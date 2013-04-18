/*******************************************************************************

FIFO/pipe task '1' solution
								Marcin Borkowski
********************************************************************************/

#define _GNU_SOURCE 
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <limits.h>
#define ERR(source) (fprintf(stderr,"%s:%d\n",__FILE__,__LINE__),\
                     perror(source),kill(0,SIGTERM),\
		     exit(EXIT_FAILURE))

typedef struct
{
	int nr;
	int p[2];
	int mesage_size;
} pair;

void create_children(int,pair*);
int sethandler(void(*)(int),int);
void sigchld_handler(int);
void usage(char*);
ssize_t bulk_write(int,char*,size_t);
ssize_t bulk_read(int,char*,size_t);
void child_work(pair*);
void parent_work(int,pair*);


int sethandler( void (*f)(int), int sigNo) {
	struct sigaction act;
	memset(&act, 0, sizeof(struct sigaction));
	act.sa_handler = f;
	if (-1==sigaction(sigNo, &act, NULL))
		return -1;
	return 0;
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

void parent_work(int n,pair *info) {
	int i = 1;
	char buffer[PIPE_BUF];
	ssize_t size;
	do{
		int j=0;
		while(1){
         printf("%d -> %d \n", info[j].p[0], info[j].p[1]);
			if((size=TEMP_FAILURE_RETRY(read(info[j].p[0],buffer,sizeof(pair))))<0)ERR("read from pipe:");
			else if(size>0){
				printf("\n<Block from subprocess %d>\n",info[j].nr);
				if(bulk_write(STDOUT_FILENO,buffer,size)<0)ERR("write on STDOUT:");
				printf("</block>\n");
				if(TEMP_FAILURE_RETRY(close(info[j].nr))) ERR("close");
				i++;
			}
			j++;
			if(j==n-1) j=0;
		}
	}while(i<n);
}


void child_work(pair *ch_p) {
	struct timespec t, tn = {0,0};
	ssize_t size;
	char buffer[PIPE_BUF];
   int r = 0;
   int i;

	srand(time(NULL)*getpid());
	t.tv_nsec = rand()%1901 + 100;
	for(t=tn;nanosleep(&t,&t););	//sleep
	
	//int r = 0;
	char c[ch_p->nr];
	for(i = 0; i< ch_p->nr;i++){
		srand(time(NULL)*getpid());
		//r = rand()%22 + 97;
      c[i] = (rand()%('z'-'a'))+'a'; //itoa(r);
		}
	
	if(size=TEMP_FAILURE_RETRY(write(ch_p->p[1],buffer,sizeof(pair)))<0)
      ERR("read from pipe:");
	if(size<sizeof(pair))ERR("read from pipe(size):");
}

void create_children(int n,pair *info) {
	while (n--) {
		switch (fork()) {
			case 0:
         {
				if(TEMP_FAILURE_RETRY(close(info[n].p[0]))) ERR("close");
				child_work(&info[n]);
				if(TEMP_FAILURE_RETRY(close(info[n].p[1]))) ERR("close");
				exit(EXIT_SUCCESS);
         }
			case -1: ERR("Fork:");
		}
	}
}

void usage(char * name){
	fprintf(stderr,"USAGE: %s  k1 k2 k3 ... kn\n",name);
	fprintf(stderr,"k1 k2 k3 ... kn - numbers from 1 to 100\n");
}

int main(int argc, char** argv) {
	if(argc<2) {
		usage(argv[0]);
		return EXIT_FAILURE;
	}
   int i;
	pair info[argc-1];
	for(i=0;i<argc-1;i++){
		info[i].mesage_size=atoi(argv[i+1]);
		if(info[i].mesage_size<1 || info[i].mesage_size>100){
			usage(argv[0]);
			return EXIT_FAILURE;
		}
      if(pipe(info[i].p)) ERR("pipe");
		info[i].nr = i;
	}
	if(sethandler(sigchld_handler,SIGCHLD)) ERR("Seting parent SIGCHLD:");
	create_children(argc-1,info);
	
	for(i=0;i<argc-1;i++){
		if(TEMP_FAILURE_RETRY(close(info[i].p[1]))) ERR("close");
	}
	parent_work(argc-1,info);
	return EXIT_SUCCESS;
}

