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
#include <sys/wait.h>

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

#define IFEQUAL_VARNAMETOSTR(x) case (x): returned = (#x); break;
#define IFEQUAL_DEFAULT default: returned = "invalid value"; break;

/*!
  \brief Converts a signal number to its string representation.
 */
const char* sigToStr(int sig)
{
   const char* returned = NULL;
   switch(sig)
   {
      IFEQUAL_VARNAMETOSTR(SIGABRT);
      IFEQUAL_VARNAMETOSTR(SIGALRM);
      IFEQUAL_VARNAMETOSTR(SIGBUS);
      IFEQUAL_VARNAMETOSTR(SIGCHLD);
      //IFEQUAL_VARNAMETOSTR(SIGCLD);
      IFEQUAL_VARNAMETOSTR(SIGCONT);
      IFEQUAL_VARNAMETOSTR(SIGFPE);
      IFEQUAL_VARNAMETOSTR(SIGINT);
      IFEQUAL_VARNAMETOSTR(SIGKILL);
      IFEQUAL_VARNAMETOSTR(SIGUSR1);
      IFEQUAL_VARNAMETOSTR(SIGUSR2);
      //IFEQUAL_VARNAMETOSTR(SIG);
      IFEQUAL_DEFAULT;
   }
   return returned;
}

#undef IFEQUAL_VARNAMETOSTR
#undef IFEQUAL_DEFAULT

#endif // MBDEV_UNIX_H
