#define _GNU_SOURCE
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <limits.h>
#include <signal.h>
#include <string.h>

#include "mbdev_unix.h"

#define MSG_SIZE (PIPE_BUF - sizeof(pid_t))

#define MIN_K 1
#define MAX_K 100

#define MIN_TIME 1
#define MAX_TIME 2

#define DEBUG_OUT 0

void usage(void)
{
	fprintf(stderr,"USAGE: param1 param2 ... paramN \n where N is in [1..100]\n");
}


//void pipe_handler(int sig) {
//	printf("[%d] received SIGPIPE and must terminate.\n", getpid());
//}

//void write_to_fifo(int fifo, int file){
//	int64_t  count;
//	char buffer[PIPE_BUF];
//	char *buf;
//	*((pid_t *)buffer)=getpid();
//	buf=buffer+sizeof(pid_t);

//	do{
//		count=bulk_read(file,buf,MSG_SIZE);
//		if(count<0){
//			perror("Read:");
//			exit(EXIT_FAILURE);
//		}
//		if(count < MSG_SIZE) memset(buf+count,0,MSG_SIZE-count);
//		if(count>0){
//			if(bulk_write(fifo,buffer,PIPE_BUF)<0){
//			perror("Write:");
//			exit(EXIT_FAILURE);
//			}
//		}
//	}while(count==MSG_SIZE);
//}

void parent_work(int fifo){
	int64_t  count;
	//int i;
	char buffer[PIPE_BUF];
	//char fname[20];
	do{
		count=bulk_read(fifo,buffer,PIPE_BUF);
		if(count<0){
			perror("Read:");
			exit(EXIT_FAILURE);
		}
		if(count>0){
			printf("<block from subproces %d>\n%s\n</block>\n",
				*(pid_t*)buffer, buffer+sizeof(pid_t));
			//printf("parent read the following: %s", buffer+sizeof(pid_t));
			//printf("writing to %d.txt\n",*((pid_t*)buffer));
			//snprintf(fname,20,"%d.txt",*((pid_t*)buffer));
			//i=filter_buffer(buffer+sizeof(pid_t),MSG_SIZE);
			//append_to_file(fname,buffer+sizeof(pid_t),i);
		}
	}while(count==PIPE_BUF);
}

void child_work(int fifo, int64_t count)
{
	//int64_t  count;
	char buffer[PIPE_BUF];
	char *buf;
	int i;

	*((pid_t *)buffer)=getpid();
	buf=buffer+sizeof(pid_t);

	if(DEBUG_OUT) fprintf(stderr,"child works\n");

	srand(getpid());
	sleepFor(rand()%(MAX_TIME-MIN_TIME+1)+MIN_TIME);

	//do{
		for(i=0;i<count;++i)
			buf[i] = (char)(rand()%(90-65+1)+65);

		if(DEBUG_OUT) fprintf(stderr,"will write %s\n", buf);

//		count=bulk_read(file,buf,MSG_SIZE);
//		if(count<0){
//			perror("Read:");
//			exit(EXIT_FAILURE);
//		}
		if(count < MSG_SIZE) memset(buf+count,0,MSG_SIZE-count);
		if(count>0){
			if(bulk_write(fifo,buffer,PIPE_BUF)<0){
			perror("Write:");
			exit(EXIT_FAILURE);
			}
		}
	//}while(count==MSG_SIZE);
}

int main(int argc, char** argv)
{
	int fifo;
	int childrenCount = argc - 1;
	int* fifos = (int*)malloc(sizeof(int)*MAX_K);
	int i;

	if(argc < 1 + MIN_K || argc > 1 + MAX_K)
	{
		usage();
		return EXIT_FAILURE;
	}

	if(DEBUG_OUT) fprintf(stderr,"children count %d\n", childrenCount);

	i = 0;
	while(childrenCount-- > 0)
	{
		int pid = fork();

		if(pid > 0)
		{
			fifos[i] = pid;

			if(DEBUG_OUT) fprintf(stderr,"added child %d\n", pid);
		}
		else if(pid == 0)
			fifos[i] = getpid();

		if(pid == 0)
		{
			int len = sizeof(char)*32;
			char* fifoName = (char*)malloc(len);
			memset(fifoName, 0, len);
			strncpy(fifoName,"fifo__", 6);
			itoa(fifos[i],fifoName+5);

			if(DEBUG_OUT) fprintf(stderr,"child will create fifo %s\n", fifoName);

			makefifo(fifoName);

			if(DEBUG_OUT) fprintf(stderr,"child will open fifo to write: %s\n", fifoName);

			fifo = openfifo(fifoName,O_WRONLY);

			child_work(fifo, atoi(argv[i+1]));

			return EXIT_SUCCESS;
		}
		i++;
	}

	//parent_work();

	for(i=0; i< argc-1; ++i)
	{
		int len = sizeof(char)*32;
		char* fifoName = (char*)malloc(len);
		memset(fifoName, 0, len);
		strncpy(fifoName,"fifo__", 6);
		itoa(fifos[i],fifoName+5);

		if(DEBUG_OUT) fprintf(stderr,"parent will create fifo %s\n", fifoName);

		// made in child but may be made later
		makefifo(fifoName);

		if(DEBUG_OUT) fprintf(stderr,"parent will open fifo to read: %s\n", fifoName);

		fifos[i] = openfifo(fifoName,O_RDONLY);

		parent_work(fifos[i]);

		closefifo(fifos[i]);

		unlinkfifo(fifoName);
	}

//	if(mkfifo(argv[1], S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP)<0)
//		if(errno!=EEXIST){
//			perror("Create fifo:");
//			exit(EXIT_FAILURE);
//		}
//	if((fifo=TEMP_FAILURE_RETRY(open(argv[1],O_WRONLY)))<0){
//			perror("Open fifo:");
//			exit(EXIT_FAILURE);
//	}
//	if((file=TEMP_FAILURE_RETRY(open(argv[2],O_RDONLY)))<0){
//			perror("Open file:");
//			exit(EXIT_FAILURE);
//	}
//	if(sethandler(pipe_handler,SIGPIPE)) {
//		perror("Seting SIGPIPE:");
//		exit(EXIT_FAILURE);
//	}
//	printf("writing...\n");
//	write_to_fifo(fifo,file);
//	if(TEMP_FAILURE_RETRY(close(file))<0){
//			perror("Close file:");
//			exit(EXIT_FAILURE);
//	}
//	if(TEMP_FAILURE_RETRY(close(fifo))<0){
//			perror("Close fifo:");
//			exit(EXIT_FAILURE);
//	}
//	printf("done.\n");
	return EXIT_SUCCESS;
}
