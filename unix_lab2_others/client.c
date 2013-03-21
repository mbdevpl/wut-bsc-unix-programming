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


typedef struct
{
	pid_t pid;
	int number;
} message;


int sethandler (void (*f)(int), int sigNo)
{
	struct sigaction act;
	memset (&act, 0, sizeof (struct sigaction));
	act.sa_handler = f;
	if (-1 == sigaction(sigNo, &act, NULL)) 
		return -1;
	else
		return 0;
}

void usage(char *name){
	fprintf(stderr,"USAGE: %s fifoname\n",name);
}

void tryBeBest(message *m, int fifo) {
		if(TEMP_FAILURE_RETRY(write(fifo,m,sizeof(message)))<sizeof(message)) return;
	
}


int main(int argc, char** argv) {
	message m;
	int fifo;
	if(argc<2) {
		usage(argv[0]);
		return EXIT_FAILURE;
	}
	srand(getpid());
	m.pid = getpid();
	m.number = rand()%10000;
	fprintf(stderr,"CLIENT[%d] SENDS NUMBER: %d\n",getpid(), m.number);

	if((fifo=TEMP_FAILURE_RETRY(open(argv[1],O_WRONLY)))<0)ERR("open:");
	tryBeBest(&m,fifo);
	if(TEMP_FAILURE_RETRY(close(fifo))<0)ERR("close:");

	return EXIT_SUCCESS;
}
