#define _GNU_SOURCE
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <limits.h>
#include <string.h>

void usage(void){
	fprintf(stderr,"USAGE: client fifo_file file \n");
}

int64_t bulk_read(int fd, char *buf, size_t count){
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

int64_t bulk_write(int fd, char *buf, size_t count){
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

void write_to_fifo(int fifo, int file){
	int64_t count;
	char buf[PIPE_BUF];
	do{
		count=bulk_read(file,buf,PIPE_BUF);
		if(count<0){
			perror("Read:");
			exit(EXIT_FAILURE);
		}
		if(count < PIPE_BUF) memset(buf+count,0,PIPE_BUF-count);
		if(count>0){
			if(bulk_write(fifo,buf,PIPE_BUF)<0){
			perror("Write:");
			exit(EXIT_FAILURE);
			}
		}
	}while(count==PIPE_BUF);
}

int main(int argc, char** argv) {
	int fifo,file;
	if(argc!=3) {
		usage();
		return EXIT_FAILURE;
	}

	if(mkfifo(argv[1], S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP)<0)
		if(errno!=EEXIST){
			perror("Create fifo:");
			exit(EXIT_FAILURE);
		}
	if((fifo=TEMP_FAILURE_RETRY(open(argv[1],O_WRONLY)))<0){
			perror("Open fifo:");
			exit(EXIT_FAILURE);
	}
	if((file=TEMP_FAILURE_RETRY(open(argv[2],O_RDONLY)))<0){
			perror("Open file:");
			exit(EXIT_FAILURE);
	}
	write_to_fifo(fifo,file);	
	if(TEMP_FAILURE_RETRY(close(file))<0){
			perror("Close file:");
			exit(EXIT_FAILURE);
	}
	if(TEMP_FAILURE_RETRY(close(fifo))<0){
			perror("Close fifo:");
			exit(EXIT_FAILURE);
	}
	return EXIT_SUCCESS;
}
