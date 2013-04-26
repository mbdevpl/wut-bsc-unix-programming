#define _GNU_SOURCE

#include <errno.h>
#include <limits.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>

#define ERR(source) (fprintf (stderr, "%s:%d\n", __FILE__, __LINE__),\
                     perror (source), kill(0, SIGINT),\
		     exit (EXIT_FAILURE))
#define BUF_SIZE 12

typedef struct
{
	pid_t pid;
	int number;
} message;

void usage(char *name){
	fprintf(stderr,"USAGE: %s fifoname\n",name);
}

int openCreateFile(char* filename){
	int fd;
	if((fd=TEMP_FAILURE_RETRY(open(filename,O_WRONLY|O_CREAT,S_IRUSR|S_IWUSR)))<0) 
		ERR("open file:");
	return fd;
}

void close_file(int f){
if(TEMP_FAILURE_RETRY(close(f)) < 0){
perror("Close file:");
exit(EXIT_FAILURE);
}
}

ssize_t bulkWrite(int fd, char *buf, size_t count){
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


int reopen(int fifo,char *fifoname){
	if(TEMP_FAILURE_RETRY(close(fifo))<0)ERR("close:");
	if((fifo=TEMP_FAILURE_RETRY(open(fifoname,O_RDONLY)))<0)ERR("open:");
	return fifo;
}

void smartChoose(char *fifoname){
	message pl[3];
	int fifo;
	int ret,i=0;
	if((fifo=TEMP_FAILURE_RETRY(open(fifoname,O_RDONLY)))<0)ERR("open:");
	while(1){
		ret=TEMP_FAILURE_RETRY(read(fifo,&pl[i],sizeof(message)));
		if(ret<0) ERR("read:");
		if(0==ret) fifo=reopen(fifo,fifoname);
		if(ret==sizeof(message)) i++;
		if(3==i){
			int num = (pl[0].number+pl[1].number+pl[2].number)%3;
			fprintf(stderr,"the winner is client %d\n",num);
			int winnerFd = openCreateFile("winner.txt");
			/*CORRECTED*/
			char buffer[BUF_SIZE];// = malloc (sizeof(pid_t));
			pid_t number=pl[num].pid;
			snprintf(buffer,BUF_SIZE,"%d\n",number);
			if(bulkWrite(winnerFd,buffer,strlen(buffer))<0){
			perror("Write:");
			exit(EXIT_FAILURE);
			}
			i=0;
			close_file(winnerFd);
			/*CORRECTED*/
		}
	}
	if(TEMP_FAILURE_RETRY(close(fifo))<0)ERR("close:");
}

int main(int argc, char** argv) {
	//message m;
	if(argc<2) {
		usage(argv[0]);
		return EXIT_FAILURE;
	}
	if(mkfifo(argv[1],O_CREAT|S_IRWXU|S_IRWXG|S_IRWXO)<0)ERR("mkfifo:");
	smartChoose(argv[1]);
	if(unlink(argv[1])<0)ERR("unlink");
	return EXIT_SUCCESS;
}
