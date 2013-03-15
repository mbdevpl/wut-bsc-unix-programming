#include <stdlib.h>
#include <stdio.h>
#include <sys/wait.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

#include "mbdev_unix.h"

#define DEBUG_OUT 1

#define SIGNAL_MIN SIGRTMIN
#define SIGNAL_MAX SIGRTMAX

#define BOMBTIME_MIN 5
#define BOMBTIME_MAX 10

volatile int children_count = 2;

volatile int terrorist = -1;

volatile int sapper = -1;

void bomb_defused_terrorist(int sig)
{
   if(DEBUG_OUT) printf("Bomb was defused with code %d!\n", sig);
   exitNormal();
}

void bomb_defused_sapper(int sig)
{
   if(DEBUG_OUT) printf("Sapper exits due to %s.\n", sigToStr(sig));

   printf("PAWNED\n");
   exitNormal();
}

void bomb_exploded_terrorist(int sig)
{
   if(DEBUG_OUT) printf("Terrorist was killed by %s!\n", sigToStr(sig));

   printf("KABOOM\n");
   exitNormal();
}

void bomb_exploded_sapper(int sig)
{
   if(DEBUG_OUT) printf("Sapper was killed by %s!\n", sigToStr(sig));
   exitNormal();
}

void handler_sigchld(int sig)
{
   pid_t pid;
   int notified;

   notified = 0;
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
         --children_count;

         if(pid == sapper)
         {
            if(DEBUG_OUT) printf("There was a note on the table: \"Sorry for everything. Best regards, %d.\"\n", pid);

            sapper = -1;
         }

         if(pid == terrorist)
         {
            if(DEBUG_OUT) printf("There was a note on the table: \"I've had enough. Best regards, %d.\"\n", pid);

            if(sapper > 0 && !notified)
            {
               if(DEBUG_OUT) printf("Bomb is no more: either exploded or was defused.\n");
               notified = 1;
               if(kill(0, SIGUSR1) < 0)
                  exitWithError("main kill-SIGUSR1");
            }
         }

      }
   }
}

void work_terrorist()
{
   int sig;
   int t;

   if(DEBUG_OUT) printf("He was just a terrorist, with pesel %d.\n", getpid());

   if(setSigHandler(bomb_exploded_terrorist, SIGUSR2))
      exitWithError("terrorist sigaction-SIGUSR2");

   srand(getpid());

   if(DEBUG_OUT) printf("Terrorist could choose one of these defusion codes: [%d,%d],\n", SIGNAL_MIN, SIGNAL_MAX);

   sig = SIGNAL_MIN + rand()%(SIGNAL_MAX-SIGNAL_MIN+1);
   if(DEBUG_OUT) printf("he picked %d...\n", sig);

   if(setSigHandler(bomb_defused_terrorist, sig))
      exitWithError("terrorist sigaction-custom");

   if(DEBUG_OUT) printf("Terrorist could set the timer anywhere between: [%d,%d],\n", BOMBTIME_MIN, BOMBTIME_MAX);

   t = BOMBTIME_MIN + rand()%(BOMBTIME_MAX-BOMBTIME_MIN+1);
   if(DEBUG_OUT) printf("he set it to %d seconds...\n", t);

   sleepFor(t);

   if(sapper > 0) // this must be true if sapper was forked before the terrorist
      if(kill(sapper, SIGUSR2) < 0)
         exitWithError("terrorist kill-SIGUSR2");

   //for(t=100;t>0;t=usleep(t)); //useless due to the fact that sigchld is queued

   if(kill(0, SIGUSR2) < 0)
      exitWithError("terrorist kill-SIGUSR2");
}

void work_sapper()
{
   int sig;

   if(DEBUG_OUT) printf("He was just a sapper with pesel %d.\n", getpid());

   if(setSigHandler(bomb_defused_sapper, SIGUSR1))
      exitWithError("sapper sigaction-SIGUSR1");

   if(setSigHandler(bomb_exploded_sapper, SIGUSR2))
      exitWithError("sapper sigaction-SIGUSR2");

   for(sig = SIGNAL_MIN; sig <= SIGNAL_MAX; ++sig)
   {
      sleepFor(1);
      if(DEBUG_OUT) printf("Sapper tries to defuse the bomb using code %d...\n", sig);
      if(kill(0, sig) < 0)
         exitWithError("sapper kill-custom");

      if(DEBUG_OUT && sig == SIGNAL_MIN + BOMBTIME_MIN)
         printf("Will he find the right code before it is too late?\n");
   }
}

void work_main()
{
   for(;;)
   {
      sleepFor(1);
      if(children_count == 0)
         return;
   }
}

int main(void)
{
   int sig;

   if(DEBUG_OUT) printf("Story written with signals, by Mateusz Bysiek.\n\n");

   for(sig = SIGNAL_MIN; sig <= SIGNAL_MAX; ++sig)
   {
      if(setSigHandler(SIG_IGN, sig))
         exitWithError("main sigaction-custom");
   }
   if(setSigHandler(SIG_IGN, SIGUSR1))
      exitWithError("main sigaction-SIGUSR1");
   if(setSigHandler(SIG_IGN, SIGUSR2))
      exitWithError("main sigaction-SIGUSR2");
   if(setSigHandler(handler_sigchld, SIGCHLD))
      exitWithError("main sigaction-SIGCHLD");

   sapper = fork();
   if(sapper == 0)
   {
      work_sapper();
      exitNormal();
   }
   else if(sapper < 0)
      exitWithError("main fork-sapper");

   terrorist = fork();
   if(terrorist == 0)
   {
      work_terrorist();
      exitNormal();
   }
   else if(terrorist < 0)
      exitWithError("main fork-terrorist");

   work_main();

   if(DEBUG_OUT) printf("And everybody was dead ever after, because the signals got them.\n");

   exitNormal();

   return 0;
}
