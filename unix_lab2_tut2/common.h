#include 


typedef struct
{
	pid_t pid;
	int number;
} message;


int openFifo()
{
	if((fifo=TEMP_FAILURE_RETRY(open(fifoname,O_RDONLY)))<0)ERR("open:");
	return fifo;
}