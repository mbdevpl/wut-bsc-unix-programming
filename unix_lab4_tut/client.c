#define _GNU_SOURCE 
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <signal.h>
#include <netdb.h>
#define ERR(source) (perror(source),\
		     fprintf(stderr,"%s:%d\n",__FILE__,__LINE__),\
		     exit(EXIT_FAILURE))

void usage(char *name)
{
	fprintf(stderr, "USAGE: %s domain port\n", name);
}

int main(int argc, char **argv)
{
	if (argc!=3)
		usage(argv[0]);
	
	return EXIT_SUCCESS;
}
