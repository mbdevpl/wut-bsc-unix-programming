#define _GNU_SOURCE
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

void usage(void){
	fprintf(stderr,"USAGE: server fifo_file\n");
}

void read_from_fifo(int fifo){
	ssize_t count;
	char c;
	do{
		count=TEMP_FAILURE_RETRY(read(fifo,&c,1));
		if(count<0){
			perror("Read:");
			exit(EXIT_FAILURE);
		}
		if(count>0){
			if(TEMP_FAILURE_RETRY(write(STDOUT_FILENO,&c,1))<0){
			perror("Write:");
			exit(EXIT_FAILURE);
			}
		}
	}while(count>0);
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
