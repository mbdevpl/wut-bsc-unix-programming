#include <stdlib.h>
#include <stdio.h>
#include <sys/wait.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

#include "mbdev_unix.h"

#define DEBUG_OUT 0

// use this to manipualte probability of police/terror winning
#define CUSTOM_SIG_MIN SIGRTMIN
#define CUSTOM_SIG_MAX SIGRTMAX
//#define CUSTOM_SIG_MAX SIGRTMIN+2

int sethandler( void (*f)(int), int sigNo);
void child_handler(int sig);
void sigchld_handler(int sig);

volatile sig_atomic_t last_signal = 0; // just an int

volatile int bomb_defused = 0;

volatile int bomb_exploded = 0;

volatile int notified = 0;

volatile int children_count = 2;

void bomb_defused_handler(int sig)
{
    printf("terrorist %d quits, bomb was defused using %d\n", getpid(), sig);
    bomb_defused = 1;
    exit(EXIT_SUCCESS);
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
                //last_signal = SIGCHLD;
                --children_count;
                if(DEBUG_OUT) printf("Child removed, %d remain\n", children_count);
                if(notified)
                    return;
                //send 1 to all children
                if (kill(0, SIGUSR1)<0)
                {
                    perror("bomb defused feedback send");
                    exit(EXIT_FAILURE);
                }
                else
                {
                    if(DEBUG_OUT) printf("sent USR1 to all\n");
                    notified = 1;
                }
                return;
            }
            perror("waitpid:");
            exit(EXIT_FAILURE);
        }
        if(pid > 0)
        {
            if(DEBUG_OUT) printf("I leave. Best regards, %d\n", getpid());
        }
    }
}

void bomb_feedback_handler(int sig)
{
    if(DEBUG_OUT) printf("I was killed. Best regards, %d\n", getpid());

    if(bomb_defused)
        fprintf(stderr,"PAWNED\n");
    exit(EXIT_SUCCESS);
}

void work_terrorist(void)
{
    int signum;
    int t;

    if(DEBUG_OUT) printf("I am just a terrorist, pesel=%d\n", getpid());

    // set random defusing signal
    srand(getpid());
    signum = CUSTOM_SIG_MIN + (rand()%(CUSTOM_SIG_MAX-CUSTOM_SIG_MIN+1));
    if(sethandler(bomb_defused_handler,signum))
    {
        perror("Seting child SIGUSR1:");
        exit(EXIT_FAILURE);
    }
    if(DEBUG_OUT) printf("Terrorist will give up upon %d\n", signum);

    t = 5 + (rand()%6);
    if(DEBUG_OUT) printf("Terrorist will kill in %d seconds...\n", t);

    for(;t>0;t=sleep(t));
    if(DEBUG_OUT) printf("terrorist cannot wait any longer\n");

    if (kill(0, SIGUSR1)<0)
    {
        perror("children_send");
        exit(EXIT_FAILURE);
    }
    else
    {
        fprintf(stderr,"KABOOM\n");
        bomb_exploded = 1;
        exit(EXIT_SUCCESS);
    }
}

void work_police(void)
{
    int signum;
    int t;

    if(sethandler(bomb_feedback_handler,SIGUSR1))
    {
        perror("Seting terror SIGUSR1:");
        exit(EXIT_FAILURE);
    }

    if(DEBUG_OUT) printf("I am just a policeman, pesel=%d\n", getpid());

    for(signum = CUSTOM_SIG_MIN; signum <= CUSTOM_SIG_MAX; ++signum)
    {
        for(t=1;t>0;t=sleep(t));
        if(DEBUG_OUT) printf("I will try to defuse using %d...\n", signum);
        if (kill(0, signum)<0)
        {
            perror("police_signal_send");
            exit(EXIT_FAILURE);
        }

    }
    exit(EXIT_SUCCESS);
}

void work_main(void)
{
    int t;

    for(;;)
    {
        for(t=1;t>0;t=sleep(t));
        if(children_count == 0)
        {
            if(DEBUG_OUT) printf("no more children\n");
            return;
        }
        else
            if(DEBUG_OUT) printf("some children, namely %d\n", children_count);

    }
}

int main(int argc, char** argv)
{
    int signum = 0;
    //fprintf(stderr,"heika\n");
    //return 0;

    //   int n, k, p, l;

    //   if(argc!=5)
    //   {
    //      usage();
    //      return EXIT_FAILURE;
    //   }

    //   n = atoi(argv[1]);
    //   k = atoi(argv[2]);
    //   p = atoi(argv[3]);
    //   l = atoi(argv[4]);

    //   if (n<=0 || k<=0 || p<=0 || l<=0)
    //   {
    //      usage();
    //      return EXIT_FAILURE;
    //   }

    if(DEBUG_OUT) printf("signals: [%d,%d]\n", CUSTOM_SIG_MIN, CUSTOM_SIG_MAX);

    // handle SIGCHLD from the terrorist
    if(sethandler(sigchld_handler,SIGCHLD))
    {
        perror("Seting parent SIGCHLD:");
        exit(EXIT_FAILURE);
    }
    // parent ignores all custom signals
    for(signum = CUSTOM_SIG_MIN; signum <= CUSTOM_SIG_MAX; ++signum)
    {
        if(sethandler(SIG_IGN,signum))
        {
            perror("ignoring parent signum:");
            exit(EXIT_FAILURE);
        }
    }
    // parent ignores SIGUSR1
    if(sethandler(SIG_IGN,SIGUSR1))
    {
        perror("ignoring parent SIGUSR1:");
        exit(EXIT_FAILURE);
    }
    // parent ignores SIGUSR2
    if(sethandler(SIG_IGN,SIGUSR2))
    {
        perror("ignoring parent SIGUSR2:");
        exit(EXIT_FAILURE);
    }

    // create 1-man bomb-squad
    pid_t police = fork();
    if(police == 0)
    {
        work_police();
    }
    else if(police == -1)
    {
        perror("Fork:");
        exit(EXIT_FAILURE);
    }
    else
    {
        //main work
    }

    // create terrorist
    pid_t terrorist = fork();
    if(terrorist == 0)
    {
        work_terrorist();
    }
    else if(terrorist == -1)
    {
        perror("Fork:");
        exit(EXIT_FAILURE);
    }
    else
    {
        //main work
    }


    work_main();
    return EXIT_SUCCESS;
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
