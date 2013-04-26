#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

void child_work(int l);
void create_children(int n, int l);
int sethandler( void (*f)(int), int sigNo);
void child_handler(int sig);
void sigchld_handler(int sig);
void usage(void);

volatile sig_atomic_t last_signal = 0;

int sethandler( void (*f)(int), int sigNo) {
	struct sigaction act;
	memset(&act, 0, sizeof(struct sigaction));
	act.sa_handler = f;
	if (-1==sigaction(sigNo, &act, NULL))
		return -1;
	return 0;
}

void child_handler(int sig) {
	printf("[%d] received signal %d\n", getpid(), sig);
	last_signal = sig;
}

void sigchld_handler(int sig) {
	pid_t pid;
	for(;;){
		pid=waitpid(0, NULL, WNOHANG);
		if(pid==0) return;
		if(pid<=0) {
			if(errno==ECHILD) return;
			perror("waitpid:");
			exit(EXIT_FAILURE);
		}
	}
}

void child_work(int l) {
	while(l-- > 0){
		sleep(2);
	}
	printf("[%d] Terminates \n",getpid());
}

void create_children(int n, int l) {
	while (n-->0) {
		switch (fork()) {
			case 0:
				if(sethandler(child_handler,SIGUSR1)) {
					perror("Seting child SIGUSR1:");
					exit(EXIT_FAILURE);
				}
				if(sethandler(child_handler,SIGUSR2)) {
					perror("Seting child SIGUSR2:");
					exit(EXIT_FAILURE);
				}
				child_work(l);
				exit(EXIT_SUCCESS);

			case -1:
				perror("Fork:");
				exit(EXIT_FAILURE);

		}
	}
}

void usage(void){
	fprintf(stderr,"USAGE: signals n k p l\n");
	fprintf(stderr,"n - number of children\n");
	fprintf(stderr,"k - Interval before SIGUSR1\n");
	fprintf(stderr,"p - Interval before SIGUSR2\n");
	fprintf(stderr,"l - lifetime of child in cycles\n");
}

int main(int argc, char** argv) {
	int n, k, p, l;

	if(argc!=5) {
		usage();
		return EXIT_FAILURE;
	}

	n = atoi(argv[1]);
	k = atoi(argv[2]);
	p = atoi(argv[3]);
	l = atoi(argv[4]);

	if (n<=0 || k<=0 || p<=0 || l<=0) {
		usage();
		return EXIT_FAILURE;
	}

	if(sethandler(SIG_IGN,SIGUSR1)) {
		perror("Seting parent SIGUSR1:");
		exit(EXIT_FAILURE);
	}
	if(sethandler(sigchld_handler,SIGCHLD)) {
		perror("Seting parent SIGCHLD:");
		exit(EXIT_FAILURE);
	}
	create_children(n, l);
	
	for(;;){
		if (kill(0, SIGUSR1)<0){
			perror("children_send");
			exit(EXIT_FAILURE);
		}
		sleep(1);
	}

	return EXIT_SUCCESS;
}