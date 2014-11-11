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
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <limits.h>
#include <signal.h>
#include <time.h>

#ifndef TEMP_FAILURE_RETRY
/*!
 * \brief Used to uninterrupted read/write.
 *
 * Evaluate x, and repeat as long as it returns -1 with `errno' set to EINTR.
 *
 * copied from unistd.h, LGPL license
 */
# define TEMP_FAILURE_RETRY(expression) \
	(__extension__ \
	({ long int __result; \
	do __result = (long int) (expression); \
	while (__result == -1L && errno == EINTR); \
	__result; }))
#endif

#define ERR(source) (fprintf(stderr,"%s:%d\n",__FILE__,__LINE__), \
	perror(source),kill(0,SIGTERM), \
	exit(EXIT_FAILURE))

void reverse(char* s);
const char* itoa(int n, char* s);

int setSigHandler( void (*f)(int), int sigNo);
const char* sigToStr(int sig);

void exitWithError(const char* errorMsg);
void exitNormal();

void sleepFor(int t);
void milisleepFor(int t);

int64_t bulk_read(int fd, char* buf, size_t count);
int64_t bulk_write(int fd, char* buf, size_t count);

void makefifo(char* arg);
int openfifo(char* arg, int flags);
void unlinkfifo(char* arg);
void closefifo(int fifo);

#endif // MBDEV_UNIX_H
