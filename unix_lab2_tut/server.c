#define _GNU_SOURCE
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <limits.h>
#include <ctype.h>

void usage(void){
	fprintf(stderr,"USAGE: server fifo_file\n");
}

int64_t  bulk_read(int fd, char *buf, size_t count){
	int c;
	size_t len=0;
	do{
		c=TEMP_FAILURE_RETRY(read(fd,buf,count));
		if(c<0) return c;
		if(c==0) return len; //EOF
		buf+=c;
		len+=c;
		count-=c;
	}while(count>0);
	return len ;
}

void read_from_fifo(int fifo){
	int64_t  count, i;
	char buffer[PIPE_BUF];
	do{
		count=bulk_read(fifo,buffer,PIPE_BUF);
		if(count<0){
			perror("Read:");
			exit(EXIT_FAILURE);
		}
		if(count>0){
			printf("\nPID:%d-------------------------------------\n",*((pid_t*)buffer)); //this is not low level IO
			for(i=sizeof(pid_t);i<PIPE_BUF;i++)
				if(isalnum(buffer[i]))
						if(TEMP_FAILURE_RETRY(write(STDOUT_FILENO,buffer+i,1))<0){
						perror("Write:");
						exit(EXIT_FAILURE);
						}
		}
	}while(count==PIPE_BUF);
}

int main(int argc, char** argv) {
	int fifo;
	if(argc!=2) {
		usage();
		return EXIT_FAILURE;
	}

	if(mkfifo(argv[1], S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP)<0)
		if(errno!=EEXIST){
			perror("Create fifo:");
			exit(EXIT_FAILURE);
		}
	if((fifo=TEMP_FAILURE_RETRY(open(argv[1],O_RDONLY)))<0){
			perror("Open fifo:");
			exit(EXIT_FAILURE);
	}
	read_from_fifo(fifo);	
	if(TEMP_FAILURE_RETRY(close(fifo))<0){
			perror("Close fifo:");
			exit(EXIT_FAILURE);
	}
	return EXIT_SUCCESS;
}
