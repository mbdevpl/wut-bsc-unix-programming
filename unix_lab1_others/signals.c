#define _GNU_SOURCE 
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#define ERR(source) (fprintf(stderr,"%s:%d\n",__FILE__,__LINE__),\
                     perror(source),kill(0,SIGKILL),\
		     		     exit(EXIT_FAILURE))
#define WINDOW 100000
#define NSMULT 1000000L

/*CORRECTED*/
volatile sig_atomic_t last_signal = 0;
volatile sig_atomic_t children = 0 ;

//void create_children(int, int);
//int sethandler( void (*f)(int), int);
//void sigchld_handler(int);
//void usage();
//void parent_work(sigset_t);
//void child_work(int, int);

/*CORRECTED*/

//void child_work(int k) {
//	int t;
//	for(t=k;t>0;t=sleep(t)){
//		if (kill(getppid(), SIGUSR1)<0){
//			perror("parent_send");
//			exit(EXIT_FAILURE);
//		}
//		}	
//	printf("[%d] Terminates \n",getpid());
//}
//void parent_work(){
//	if(sethandler(parent_handler,SIGUSR1)) {
//					perror("Seting parent SIGUSR1:");
//					exit(EXIT_FAILURE);
//				}
//
//	}
//void parent_handler(int sig) {
//	printf("parent received signal %d\n", sig);
//	
//}
//void create_children(int t) {

/*CORRECTED*/
void child_work(int timeint, int sig) {	
	int randomtime;
	struct itimerval lifetime;		
	struct timespec t, tn = {0,0};
	tn.tv_nsec = timeint * NSMULT;
	srand(time(NULL)*getpid());	
	randomtime = 500 + rand()%1501;
	memset(&lifetime, 0, sizeof(struct itimerval));
	if (randomtime >= 1000)
	{
		lifetime.it_value.tv_sec = 1;
		lifetime.it_value.tv_usec = (randomtime - 1000)*1000; 
	} 
	else
		lifetime.it_value.tv_usec = randomtime * 1000;
		fprintf(stderr,"PROCESS CREATED with pid: [ %d ], interval: [ %d ] ms, lifetime: [ %d ] ms, ppid [ %d ]\n",getpid(), timeint, randomtime,getppid());
	setitimer(ITIMER_REAL,&lifetime,NULL);
	while (last_signal != SIGALRM) {
		if(kill(getppid(), sig)<0) ERR("kill:");
		for(t=tn;nanosleep(&t,&t);)
			if(EINTR!=errno) ERR("nanosleep:");
	}
	fprintf(stderr,"process [ %d ] - terminates!\n", getpid());	
	exit(EXIT_SUCCESS);
}
/*CORRECTED*/

/*CORRECTED*/
void parent_work(sigset_t oldmask) {	
	volatile int flag=0, window_open=0;
	struct itimerval wintime;
	memset(&wintime, 0, sizeof(struct itimerval));
	wintime.it_value.tv_usec=WINDOW;

	while(children){
		last_signal=0;
		if (!window_open)
		{
		    setitimer(ITIMER_REAL,&wintime,NULL);
		    window_open = 1;
		}
		sigsuspend(&oldmask);
		if(last_signal == SIGRTMIN) 
			{
				if(flag==0){
					flag=1;
				}
				else flag=3;
			}
			else if (last_signal == SIGRTMAX)
			{
				if(flag==1){
					flag=2;			
				}
				else flag=3;
			}
			else if (last_signal == SIGALRM)
			{ 
				if(flag==2){
				fprintf(stderr,"SUCCESS\n");
				}
				window_open=0;
				flag=0;
			}
		}
	}


/*CORRECTED*/
void create_children(int t1, int t2) {
	int i;	
	for (i=1;i<3;i++) {
		switch (fork()) {
			case 0:
				if (i == 1)
				{
					child_work(t1, SIGRTMIN);
					//children++;
				}
				else if (i == 2)
				{
					child_work(t2, SIGRTMAX);
					//children++;
				}
				exit(EXIT_SUCCESS);

			case -1:
				ERR("Fork:");
				exit(EXIT_FAILURE);

		} children++;
	}
}
/*CORRECTED*/

/*CORRECTED*/
int sethandler( void (*f)(int), int sigNo) {
	struct sigaction act;
	memset(&act, 0, sizeof(struct sigaction));
	act.sa_handler = f;
	if (-1==sigaction(sigNo, &act, NULL))
		return -1;
	return 0;
}
/*CORRECTED*/

void sigchld_handler(int sig) {
	pid_t pid;
	for(;;){
		pid=waitpid(0, NULL, WNOHANG);
		if(pid>0) children--;
		if(pid==0) return;
		if(pid<=0) {
			if(errno==ECHILD) return;
			perror("waitpid:");
			exit(EXIT_FAILURE);
		}
	}
}
/*CORRECTED*/
void sig_handler(int sig) {
	last_signal = sig;
}
/*CORRECTED*/


void usage(void){
	fprintf(stderr,"USAGE: t1 t2\n");
	fprintf(stderr,"t1 - time in ms [50-100]\n");
	fprintf(stderr,"t2 - time in ms [50-100]\n");
}

int main(int argc, char** argv) {
	/*CORRECTED*/
	sigset_t mask, oldmask;
	int t1,t2;
	
	if(argc!=3) {
		usage();
		return EXIT_FAILURE;
	}

	t1 = atoi(argv[1]);
	t2 = atoi(argv[2]);

	if(t1<50 || t1>100 || t2<50 || t2>100){
		usage();
		return EXIT_FAILURE;
	}	
	
	/*CORRECTED*/
	
	/*CORRECTED*/
	sigemptyset(&mask);
	if(sethandler(sigchld_handler,SIGCHLD)) ERR("Setting SIGCHILD");
	sigaddset (&mask, SIGCHLD);
	if(sethandler(sig_handler,SIGRTMIN)) ERR("Setting SIGRTMIN");
	sigaddset (&mask, SIGRTMIN);
	if(sethandler(sig_handler,SIGRTMAX)) ERR("Setting SIGRTMAX:");
	sigaddset (&mask, SIGRTMAX);
	if(sethandler(sig_handler,SIGALRM)) ERR("Setting SIGALRM:");
	sigaddset (&mask, SIGALRM);
	/*CORRECTED*/
		
	//parent_work();
	//create_children(t1);
	//create_children(t2);

	create_children(t1,t2);
	sigprocmask (SIG_BLOCK, &mask, &oldmask);
	parent_work(oldmask);
	sigprocmask (SIG_UNBLOCK, &mask, NULL);

	return EXIT_SUCCESS;
}