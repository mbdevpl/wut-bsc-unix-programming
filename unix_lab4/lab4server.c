#include "mbdev_unix.h"

#include <sys/un.h>
#include <sys/socket.h>
#include <netdb.h>

#include <pthread.h>
#include <semaphore.h>

#define DEBUG_OUT 0

#define BACKLOG 3

#define MSGSIZE 1000

volatile sig_atomic_t last_signal = 0;

typedef struct
{
	//int id;
	//int* idlethreads;
	int* socketReceive;
	//int *condition;
	//pthread_cond_t *cond;
	//pthread_mutex_t *mutex;
	//sem_t *semaphore;
} thread_arg;

void handlerSigint(int sig)
{
	last_signal=sig;
}

//int bind_local_socket(char *name)
//{
//	struct sockaddr_un addr;
//	int socketfd;
//	if(unlink(name) < 0 && errno != ENOENT) ERR("unlink");
//	socketfd = makeSocket(PF_UNIX,SOCK_STREAM);
//	memset(&addr, 0, sizeof(struct sockaddr_un));
//	addr.sun_family = AF_UNIX;
//	strncpy(addr.sun_path,name,sizeof(addr.sun_path)-1);
//	if(bind(socketfd,(struct sockaddr*) &addr,SUN_LEN(&addr)) < 0)  ERR("bind");
//	if(listen(socketfd, BACKLOG) < 0) ERR("listen");
//	return socketfd;
//}

int bind_inet_socket(uint16_t port,int type)
{
	struct sockaddr_in addr;
	int socketfd;
	int t=1;

	socketfd = makeSocket(PF_INET,type);
	memset(&addr, 0, sizeof(struct sockaddr_in));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	if (setsockopt(socketfd, SOL_SOCKET, SO_REUSEADDR,&t, sizeof(t)))
		ERR("setsockopt");

	if(bind(socketfd,(struct sockaddr*) &addr,sizeof(addr)) < 0)
	{
		switch(type)
		{
		case SOCK_STREAM:
			{
				if(errno == EADDRINUSE)
				{
					if(DEBUG_OUT) printf("EADDRINUSE\n");
					if(TEMP_FAILURE_RETRY(close(socketfd))<0)
						ERR("close");
					return -1;
				}
			} break;
		default: ERR("bind");
		}
	}

	if(SOCK_STREAM==type)
	{
		if(listen(socketfd, BACKLOG) < 0)
			ERR("listen");
	}

	return socketfd;
}

//void sendingProcessWork(int socket, sigset_t* mask)
//{
//	exit(EXIT_SUCCESS);
//}

//void createSendingProcess(int socket, sigset_t* mask)
//{
//	int descriptor;

//	if((descriptor = TEMP_FAILURE_RETRY(accept(socket, NULL, NULL))) < 0)
//		ERR("accept");

//	switch (fork()) {
//	case 0:
//		{
//			sendingProcessWork(descriptor, mask);
//		} break;
//	case -1: ERR("Fork:");
//	}

//	if(TEMP_FAILURE_RETRY(close(descriptor))<0)
//		ERR("close descriptor");
//}

void* threadWork(void *arg)
{
	if(DEBUG_OUT) printf("new connection\n");

	thread_arg targ;
	memcpy(&targ, arg, sizeof(targ));

	struct sockaddr addr;
	socklen_t size=sizeof(addr);
	int descriptor;
	int i;

	fd_set rfds;
	char buf[MSGSIZE];

	if((descriptor = TEMP_FAILURE_RETRY(accept(*(targ.socketReceive), NULL, NULL))) < 0)
		ERR("accept");

	while(1)
	{
		FD_ZERO(&rfds);
		FD_SET(descriptor, &rfds);

		if(TEMP_FAILURE_RETRY(select(descriptor+1, &rfds, NULL, NULL, NULL)) <= 0)
			exitWithError("select");

		memset(buf, 0, MSGSIZE);
		if(TEMP_FAILURE_RETRY(recvfrom(descriptor, buf, sizeof(char) * MSGSIZE, 0, &addr, &size)<0))
			ERR("recvfrom:");

		if(!buf[0])
			break;

		int length = 0;
		if(buf[MSGSIZE-1])
		{
			length = MSGSIZE;
			buf[MSGSIZE-1] = 0;
		}
		else
			length = strlen(buf);

		if(DEBUG_OUT) printf("buf length is %d\n", length);

		int iCurr = 0;
		char tempBuf[MSGSIZE];
		for(i=0; i<length; ++i)
		{
			if(DEBUG_OUT) printf("i=%d iCurr=%d char=%c\n", i, iCurr, buf[i]);
			if(buf[i] == ' ' || buf[i] == '\n' || buf[i] == '\r')
			{
				memset(tempBuf, 0, iCurr + 2);
				strncpy(tempBuf, (const char*)(buf + i - (iCurr)), iCurr);
				reverse(tempBuf);
				strncpy((char*)(buf + i - (iCurr)), tempBuf, iCurr);
				if(DEBUG_OUT) printf("reversed %d chars, %s\n", iCurr, tempBuf);
				iCurr = 0;
				continue;
			}
			tempBuf[iCurr] = buf[i];
			++iCurr;

		}

		if(DEBUG_OUT) printf("received:\n%s\n", buf);

		bulk_write(descriptor, buf, length);

	}

	return NULL;
}

void startNewThread(int socketReceive)
{
	pthread_t thread;
	thread_arg args;

	args.socketReceive = &socketReceive;

	if (pthread_create(&thread, NULL, threadWork, (void *) &args) != 0)
		ERR("pthread_create");

//	switch(fork())
//	{
//	case 0:
//	{
//		//FD_CLR(socketReceive, &rfds);
//	} break;
//	case -1: exitWithError("fork");
//	default:
//	{
//		threadWork(&args);
//	} break;

//	}
}

void serverWork(int portReceive)
{
	//int16_t portSend = 0;
	//int16_t portSendOld;
	int socketReceive;
	//int socketSend;

	sigset_t mask, oldmask;

	fd_set rfdsDef, rfds;
	int fdmax;

	sigemptyset(&mask);
	sigaddset(&mask, SIGINT);
	sigprocmask(SIG_BLOCK, &mask, &oldmask);

	socketReceive = bind_inet_socket(portReceive, SOCK_STREAM);
	fdmax = socketReceive;

	FD_ZERO(&rfdsDef);
	FD_SET(socketReceive, &rfdsDef);

	while(!last_signal)
	{
		rfds = rfdsDef;
		if(pselect(fdmax+1, &rfds, NULL, NULL, NULL, &oldmask) <= 0)
			continue;

		if(FD_ISSET(socketReceive, &rfds))
		{
			startNewThread(socketReceive);
		}

	}

	kill(0, SIGINT);

	//closeSocket(socketSend);
	closeSocket(socketReceive);
}

void usage()
{
	fprintf(stderr, "USAGE: ./server port\n");
	exitWithError("usage");
}

int main(int argc, char** argv)
{
	int port_num = 0;

	if(argc != 2)
		usage();
	port_num = atoi(argv[1]);
	if(port_num == 0)
		exitWithError("atoi parse error");

	setSigHandler(handlerSigchldDefault, SIGCHLD);
	setSigHandler(handlerSigint, SIGINT);
	setSigHandler(SIG_IGN, SIGPIPE);

	if(DEBUG_OUT) printf("server starts at port %d\n", port_num);

	serverWork(port_num);

	while(TEMP_FAILURE_RETRY(wait(NULL)) >= 0)
		;

	if(DEBUG_OUT) printf("server exits\n");
	return EXIT_SUCCESS;
}
