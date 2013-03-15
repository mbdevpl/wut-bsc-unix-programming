#include <stdlib.h>
#include <stdio.h>
#include <sys/wait.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

#include "mbdev_unix.h"

void child_work(int l);
void create_children(int n, int l);
void parent_work(int k, int p);
int sethandler( void (*f)(int), int sigNo);
void child_handler(int sig);
void sigchld_handler(int sig);
void usage(void);

volatile sig_atomic_t last_signal = 0; // just an int

void child_handler(int sig)
{
   printf("[%d] received signal %s\n", getpid(), sigToStr(sig));
   last_signal = sig;
}

void sigchld_handler(int sig)
{
   pid_t pid;
   for(;;)
   {
      pid=waitpid(0, NULL, WNOHANG);
      if(pid==0)
         return;
      if(pid<=0)
      {
         if(errno==ECHILD)
         {
            last_signal = SIGCHLD;
            return;
         }
         perror("waitpid:");
         exit(EXIT_FAILURE);
      }
   }
}

void child_work(int l)
{
   int t,tt;

   //srand(getpid());
   t = 5; //rand()%6+5;

   while(l-- > 0)
   {
      for(tt=t;tt>0;tt=sleep(tt));
      if (last_signal == SIGUSR1)
         printf("becikowe dla %d\n", getpid());
      else
         printf("nie ma, %d\n", getpid());
   }
   printf("[%d] Terminates \n",getpid());
}

void parent_work(int k, int p)
{
   int t ;
   for(;;)
   {
      for(t=k;t>0;t=sleep(t));
      if (kill(0, SIGUSR1)<0)
      {
         perror("children_send");
         exit(EXIT_FAILURE);
      }
		else
			printf("%d sent a %s to its gruop\n", getpid(), sigToStr(SIGUSR1));
		
      if(last_signal == SIGCHLD)
         return;
		
      for(t=p;t>0;t=sleep(t));
      if (kill(0, SIGUSR2)<0)
      {
         perror("children_send");
         exit(EXIT_FAILURE);
      }
		else
			printf("%d sent a %s to its gruop\n", getpid(), sigToStr(SIGUSR2));

      if(last_signal == SIGCHLD)
         return;
   }
}

void create_children(int n, int l)
{
   while (n-->0)
   {
      switch (fork())
      {
      case 0:
			printf("I am just a child, pesel=%d\n", getpid());
         if(sethandler(child_handler,SIGUSR1))
         {
            perror("Seting child SIGUSR1:");
            exit(EXIT_FAILURE);
         }
         if(sethandler(child_handler,SIGUSR2))
         {
            perror("Seting child SIGUSR2:");
            exit(EXIT_FAILURE);
         }
         child_work(l);
         exit(EXIT_SUCCESS);

      case -1:
         perror("Fork:");
         exit(EXIT_FAILURE);

      }
		if(n > 0)
			printf("%d says next!\n", getpid());
		else
			printf("%d says no more!\n", getpid());
   }
}

int main(int argc, char** argv)
{
   //fprintf(stderr,"heika\n");
   //return 0;

   int n, k, p, l;

   if(argc!=5)
   {
      usage();
      return EXIT_FAILURE;
   }

   n = atoi(argv[1]);
   k = atoi(argv[2]);
   p = atoi(argv[3]);
   l = atoi(argv[4]);

   if (n<=0 || k<=0 || p<=0 || l<=0)
   {
      usage();
      return EXIT_FAILURE;
   }

   if(sethandler(sigchld_handler,SIGCHLD))
   {
      perror("Seting parent SIGCHLD:");
      exit(EXIT_FAILURE);
   }
   if(sethandler(SIG_IGN,SIGUSR1))
   {
      perror("Seting parent SIGUSR1:");
      exit(EXIT_FAILURE);
   }
   if(sethandler(SIG_IGN,SIGUSR2))
   {
      perror("Seting parent SIGUSR2:");
      exit(EXIT_FAILURE);
   }
   create_children(n, l);
   parent_work(k, p);

   return EXIT_SUCCESS;
}

void usage(void)
{
   fprintf(stderr,"USAGE: signals n k p l\n");
   fprintf(stderr,"n - number of children\n");
   fprintf(stderr,"k - Interval before SIGUSR1\n");
   fprintf(stderr,"p - Interval before SIGUSR2\n");
   fprintf(stderr,"l - lifetime of child in cycles\n");
}

int sethandler( void (*f)(int), int sigNo)
{
   struct sigaction act;
   memset(&act, 0, sizeof(struct sigaction));
   act.sa_handler = f;
   if (-1==sigaction(sigNo, &act, NULL))
      return -1;
   return 0;
}
