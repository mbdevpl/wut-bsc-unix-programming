#ifndef MBDEV_UNIX_H
#define MBDEV_UNIX_H
/**
    Copyright 2013 Mateusz Bysiek
    mb@mbdev.pl

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
 */

#include <stdlib.h>
#include <stdio.h>
#include <sys/wait.h>
#include <unistd.h>
#include <string.h>

void reverse(char* s);
const char* itoa(int n, char* s);
int setSigHandler( void (*f)(int), int sigNo);
void exitWithError(const char* errorMsg);
const char* sigToStr(int sig);

/*!
  \brief Used to uninterrupted read/write.

  Evaluate x, and repeat as long as it returns -1 with `errno' set to EINTR.

  copied from unistd.h, LGPL license
  */
#ifndef TEMP_FAILURE_RETRY
#define TEMP_FAILURE_RETRY(x) \
   (__extension__ ( \
{ \
   long int __result; \
   do \
   result = (long int)(x); \
   while (__result == -1L && errno == EINTR); \
} \
   ))
#endif

/*!
 * \brief reverse string s in place
 *
 * used by itoa
 *
 * source: first edition of Kernighan and Ritchie's The C Programming Language, around page 60
 */
void reverse(char s[])
{
   int i, j;
   char c;

   for (i = 0, j = strlen(s)-1; i<j; i++, j--) {
      c = s[i];
      s[i] = s[j];
      s[j] = c;
   }
}


/*!
 * \brief convert n to characters in s
 *
 * copied here because itoa is not a standard function
 *
 * source: first edition of Kernighan and Ritchie's The C Programming Language, around page 60
 */
const char* itoa(int n, char* s)
{
   int i, sign;

   if ((sign = n) < 0)  /* record sign */
      n = -n;          /* make n positive */
   i = 0;
   do {       /* generate digits in reverse order */
      s[i++] = n % 10 + '0';   /* get next digit */
   } while ((n /= 10) > 0);     /* delete it */
   if (sign < 0)
      s[i++] = '-';
   s[i] = '\0';
   reverse(s);
   return s;
}

/*!
 * \brief sets the handler for the specified signal
 *
 * copied from my unix programming classes materials
 *
 * \param f handler
 * \param sigNo signal number
 * \return -1 if error occured while setting up handler, 0 otherwise
 */
int setSigHandler( void (*f)(int), int sigNo)
{
   struct sigaction act;
   memset(&act, 0, sizeof(struct sigaction));
   act.sa_handler = f;
   if (-1==sigaction(sigNo, &act, NULL))
      return -1;
   return 0;
}

/*!
 * \brief Exits the process with EXIT_FAILURE and message.
 */
void exitWithError(const char* errorMsg)
{
   perror(errorMsg);
   exit(EXIT_FAILURE);
}

/*!
 * \brief Exits the process normallly (i.e. with EXIT_SUCCESS).
 */
void exitNormal()
{
   exit(EXIT_SUCCESS);
}

void sleepFor(int t)
{
   for(;t>0;t=sleep(t));
}

//#define IFEQUAL_VARNAMETOSTR(x) case (x): returned = (#x); break;
#define IFEQUAL_VARNAMETOSTR(x) else if(sig == x) returned = (#x)

/*!
  \brief Converts a signal number to its string representation.
 */
const char* sigToStr(int sig)
{
   const char* returned;
//   switch(sig)
//   {
   if(sig < 0) returned = "invalid signal";
   IFEQUAL_VARNAMETOSTR(SIGABRT);
   IFEQUAL_VARNAMETOSTR(SIGALRM);
   IFEQUAL_VARNAMETOSTR(SIGBUS);
   IFEQUAL_VARNAMETOSTR(SIGCHLD);
   IFEQUAL_VARNAMETOSTR(SIGCLD); // same as SIGCHLD
   IFEQUAL_VARNAMETOSTR(SIGCONT);
   IFEQUAL_VARNAMETOSTR(SIGFPE);
   IFEQUAL_VARNAMETOSTR(SIGINT);
   IFEQUAL_VARNAMETOSTR(SIGKILL);
   IFEQUAL_VARNAMETOSTR(SIGPIPE);
   IFEQUAL_VARNAMETOSTR(SIGPOLL);
   IFEQUAL_VARNAMETOSTR(SIGPROF);
   IFEQUAL_VARNAMETOSTR(SIGPWR);
   IFEQUAL_VARNAMETOSTR(SIGQUIT);
   IFEQUAL_VARNAMETOSTR(SIGRTMAX); // not a constant, cannot use switch
   IFEQUAL_VARNAMETOSTR(SIGRTMIN); // not a constant, cannot use switch
   IFEQUAL_VARNAMETOSTR(SIGSEGV);
   IFEQUAL_VARNAMETOSTR(SIGSTKFLT);
   IFEQUAL_VARNAMETOSTR(SIGSTKSZ);
   IFEQUAL_VARNAMETOSTR(SIGSTOP);
   IFEQUAL_VARNAMETOSTR(SIGSYS);
   IFEQUAL_VARNAMETOSTR(SIGTERM);
   IFEQUAL_VARNAMETOSTR(SIGTRAP);
   IFEQUAL_VARNAMETOSTR(SIGTSTP);
   IFEQUAL_VARNAMETOSTR(SIGTTIN);
   IFEQUAL_VARNAMETOSTR(SIGTTOU);
   IFEQUAL_VARNAMETOSTR(SIGUNUSED); // same as SIGSYS
   IFEQUAL_VARNAMETOSTR(SIGURG);
   IFEQUAL_VARNAMETOSTR(SIGUSR1);
   IFEQUAL_VARNAMETOSTR(SIGUSR2);
   IFEQUAL_VARNAMETOSTR(SIGVTALRM);
   IFEQUAL_VARNAMETOSTR(SIGWINCH);
   IFEQUAL_VARNAMETOSTR(SIGXCPU);
   IFEQUAL_VARNAMETOSTR(SIGXFSZ);
   //IFEQUAL_VARNAMETOSTR(SIG);
   //default:
   else
   {
      if(sig >= SIGRTMIN && sig <= SIGRTMAX)
         returned = "custom signal";

      char* temp = (char*)malloc(4*sizeof(char));
      returned = itoa(sig, temp);
   }
   //}
   return returned;
}

#undef IFEQUAL_VARNAMETOSTR

#endif // MBDEV_UNIX_H
