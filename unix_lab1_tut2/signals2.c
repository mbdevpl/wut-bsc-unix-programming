#include <stdio.h>
#include <stdlib.h>

void child_work();
void create_children(int n);
void usage(void);

void child_work(void) {

	printf("[%d] Terminates \n",getpid());
}

void create_children(int n ) {
	while (n-->0) {
		switch (fork()) {
			case 0:
				child_work();
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

	return EXIT_SUCCESS;
}
