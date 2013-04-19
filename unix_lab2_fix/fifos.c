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

//#define MIN_TIME 1
#define MIN_TIME 1000
//#define MAX_TIME 2
#define MAX_TIME 2000

#define DEBUG_OUT 0

// undefine this to restore sequential reading
#define USE_SELECT

void usage(void)
{
	fprintf(stderr,"USAGE: param1 param2 ... paramN \n where N is in [1..100]\n");
}

//void _handler_sigpipe(int sig) {
//   printf("[%d] received SIGPIPE and must terminate.\n", getpid());
//}

void handler_sigchld(int sig)
{
   pid_t pid;
   for(;;)
   {
      pid = waitpid(0, NULL, WNOHANG);
      if(pid == 0)
         return;
      else if(pid < 0)
      {
         if(errno == ECHILD)
            return; // last child exitted
         exitWithError("main waitpid");
      }
      else
      {
         if(DEBUG_OUT) printf("%d exits.\n", pid);
      }
   }
}

//void parent_work(int fifo)
void parent_work(/*int fifo,*/ int* pipes, int expected)
{
	int64_t  count;
	char buffer[PIPE_BUF];
	do
   {
		//count=bulk_read(fifo,buffer,PIPE_BUF);
		//if(count < 0)
      //   exitWithError("fifo read");

      if(DEBUG_OUT) fprintf(stderr,"parent has pipes: read=%d and write=%d\n", pipes[0], pipes[1]);
      count=bulk_read(pipes[0],buffer,PIPE_BUF);
		if(count < 0)
         exitWithError("pipe read");

		if(count > 0)
      {
			printf("<block from subproces %d>\n%s\n</block>\n",
				*(pid_t*)buffer, buffer+sizeof(pid_t));
		}

      if(count >= expected)
         break;
      else if(DEBUG_OUT) fprintf(stderr,"count=%lld is less than expected=%d\n", count, expected);

      //if(DEBUG_OUT) fprintf(stderr,"wat.\n");
	}
   while(count == PIPE_BUF);
}

//void child_work(int fifo, int64_t count)
void child_work(/*int fifo,*/ int64_t count, int* pipes)
{
	char buffer[PIPE_BUF];
	char *buf;
	int i;

	*((pid_t *)buffer)=getpid();
	buf=buffer+sizeof(pid_t);

	if(DEBUG_OUT) fprintf(stderr,"%d works\n", getpid());

	srand(getpid());
	//sleepFor(rand()%(MAX_TIME-MIN_TIME+1)+MIN_TIME);
   milisleepFor(rand()%(MAX_TIME-MIN_TIME+1)+MIN_TIME);

   for(i=0;i<count;++i)
      buf[i] = (char)(rand()%('z'-'a')+'a');

   if(DEBUG_OUT) fprintf(stderr,"%d has pipes: read=%d and write=%d\n", getpid(), pipes[0], pipes[1]);

   if(count < MSG_SIZE)
      memset(buf+count,0,MSG_SIZE-count);
   if(count > 0)
   {
      //if(bulk_write(fifo,buffer,PIPE_BUF)<0){
      //perror("Write:");
      //exit(EXIT_FAILURE);
      //}
      if(DEBUG_OUT) fprintf(stderr,"%d writes %s\n", getpid(), buf);
      if(bulk_write(pipes[1],buffer,PIPE_BUF)<0)
         exitWithError("pipe writing");

      if(TEMP_FAILURE_RETRY(close(pipes[1]))) ERR("close");
   }
}

void writersCreationLoop(int childrenCount, char** argv, int* pipes)
{
	int i;
   for(i = 0; i < childrenCount; ++i)
	{
      if(pipe(pipes + 2*i)<0)
         ERR("pipe");

		int pid = fork();

		if(pid > 0)
		{
			//fifos[i] = pid;

			if(DEBUG_OUT) fprintf(stderr,"added child %d\n", pid);

         if(TEMP_FAILURE_RETRY(close(*(pipes + 2*i + 1)))) ERR("close");
		}
		//else if(pid == 0)
			//fifos[i] = getpid();

		if(pid == 0)
		{
			//int len = sizeof(char)*32;
			//char* fifoName = (char*)malloc(len);
			//memset(fifoName, 0, len);
			//strncpy(fifoName,"fifo__", 6);
			//itoa(fifos[i],fifoName+5);

			//if(DEBUG_OUT) fprintf(stderr,"child will create fifo %s\n", fifoName);

			//makefifo(fifoName);

			//if(DEBUG_OUT) fprintf(stderr,"child will open fifo to write: %s\n", fifoName);

			//fifo = openfifo(fifoName,O_WRONLY);
         //free(fifoName);

			//child_work(fifo, atoi(argv[i+1]));
         child_work(/*fifo,*/ atoi(argv[i+1]), pipes + 2*i);

         exitNormal();
		}
	}

}

void readersLoop(int childrenCount, char** argv, int* pipes)
{
   int i, j;

#ifdef USE_SELECT
   fd_set pipesToRead;
   int read[childrenCount];
   for(i=0; i < childrenCount; ++i)
      read[i] = 0;
   int maxPipeId = *(pipes + 2*(childrenCount-1));
   if(DEBUG_OUT) fprintf(stderr,"maxPipeId is %d\n", maxPipeId);
#endif

   for(i=0; i < childrenCount; ++i)
	{
		//int len = sizeof(char)*32;
		//char* fifoName = (char*)malloc(len);
		//memset(fifoName, 0, len)   ;
		//strncpy(fifoName,"fifo__", 6);
		//itoa(fifos[i],fifoName+5);

		//if(DEBUG_OUT) fprintf(stderr,"parent will create fifo %s\n", fifoName);

		// made in child but may be made later
		//makefifo(fifoName);

		//if(DEBUG_OUT) fprintf(stderr,"parent will open fifo to read: %s\n", fifoName);

		//fifos[i] = openfifo(fifoName,O_RDONLY);

#ifdef USE_SELECT
      // prepare set of pipes to select from
      FD_ZERO(&pipesToRead);
      for(j=childrenCount-1; j>=0; --j)
         if(!read[j])
         {
            FD_SET(*(pipes + 2*j), &pipesToRead);
            if(DEBUG_OUT) fprintf(stderr,"added %d to fd_set\n", *(pipes + 2*j));
         }

      // select pipe
      int pipeCountToRead = select(maxPipeId + 1, &pipesToRead, NULL, NULL, NULL);
      if(pipeCountToRead == 0)
      {
         // somehow timeout happened...
         --i;
         continue;
      }
      else if(pipeCountToRead < 0)
      {
         if(errno == EINTR)
         {
            if(DEBUG_OUT) fprintf(stderr,"wat.\n");
            --i;
            continue;
         }
         switch(pipeCountToRead)
         {
         case ENOMEM: ERR("select() internal alloc error");
         case EBADF: ERR("ebdaf");
         case EINVAL: ERR("einval");
         default: ERR("u wat m8");
         }
      }

      // actually find a pipe that is ready to be read
      int n;
      for(n=childrenCount-1; n>=0; --n)
      {
         int currPipe = *(pipes + 2*n);
         if(FD_ISSET(currPipe, &pipesToRead))
         {
            //FD_CLR(currPipe, pipesToRead);
            if(DEBUG_OUT) fprintf(stderr,"found pipe %d\n", currPipe);

            if(currPipe == maxPipeId)
               maxPipeId = *(pipes + 2*(n-1));
            break;
         }
      }

      // at last, read
      parent_work(/*fifos[i],*/ pipes + 2*n, atoi(argv[n+1]));
      if(TEMP_FAILURE_RETRY(close(*(pipes + 2*n)))) ERR("close");
      read[n] = 1;
#else
      parent_work(/*fifos[i],*/ pipes + 2*i, atoi(argv[i+1]));
      if(TEMP_FAILURE_RETRY(close(*(pipes + 2*i)))) ERR("close");
#endif

		//closefifo(fifos[i]);

		//unlinkfifo(fifoName);
      //free(fifoName);

      //remaining = 0;
	}

   //free(fifos);
}

int main(int argc, char** argv)
{
	//int fifo;
	int childrenCount = argc - 1;
	//int* fifos = (int*)malloc(sizeof(int)*MAX_K);
   int pipes[childrenCount * 2];

	//if(argc < 1 + MIN_K || argc > 1 + MAX_K)
	if(childrenCount < MIN_K || childrenCount > MAX_K)
	{
		usage();
		return EXIT_FAILURE;
	}

	if(DEBUG_OUT) fprintf(stderr,"children count %d\n", childrenCount);

   if(setSigHandler(handler_sigchld, SIGCHLD))
      exitWithError("main sigaction-SIGCHLD");

   writersCreationLoop(childrenCount, argv, pipes);

   readersLoop(childrenCount, argv, pipes);

	return EXIT_SUCCESS;
}
