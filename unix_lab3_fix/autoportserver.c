#include "mbdev_unix.h"

#include <sys/un.h>
#include <sys/socket.h>
#include <netdb.h>

#define DEBUG_OUT 1

#define BACKLOG 3

volatile sig_atomic_t last_signal = 0;

void handlerSigint(int sig)
{
	last_signal=sig;
}

int bind_local_socket(char *name)
{
	struct sockaddr_un addr;
	int socketfd;
	if(unlink(name) < 0 && errno != ENOENT) ERR("unlink");
	socketfd = makeSocket(PF_UNIX,SOCK_STREAM);
	memset(&addr, 0, sizeof(struct sockaddr_un));
	addr.sun_family = AF_UNIX;
	strncpy(addr.sun_path,name,sizeof(addr.sun_path)-1);
	if(bind(socketfd,(struct sockaddr*) &addr,SUN_LEN(&addr)) < 0)  ERR("bind");
	if(listen(socketfd, BACKLOG) < 0) ERR("listen");
	return socketfd;
}

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

void sendingProcessWork(int socket, sigset_t* mask)
{
	exit(EXIT_SUCCESS);
}

void createSendingProcess(int socket, sigset_t* mask)
{
	int descriptor;

	if((descriptor = TEMP_FAILURE_RETRY(accept(socket, NULL, NULL))) < 0)
		ERR("accept");

	switch (fork()) {
	case 0:
		{
			sendingProcessWork(descriptor, mask);
		} break;
	case -1: ERR("Fork:");
	}

	if(TEMP_FAILURE_RETRY(close(descriptor))<0)
		ERR("close descriptor");
}

void serverWork(int portReceive)
{
	int16_t portSend = 0;
	int16_t portSendOld;
	int socketReceive;
	int socketSend;

	struct sockaddr addr;
	socklen_t size=sizeof(addr);

	sigset_t mask, oldmask;

	fd_set rfdsDef, rfds;
	int fdmax;

	sigemptyset(&mask);
	sigaddset(&mask, SIGINT);
	sigprocmask(SIG_BLOCK, &mask, &oldmask);

	socketReceive = bind_inet_socket(portReceive, SOCK_DGRAM);
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
			if(DEBUG_OUT) printf("current port for sending is %d\n", portSend);

			portSendOld = portSend;
			if(TEMP_FAILURE_RETRY(recvfrom(socketReceive, (char*)(&portSend), sizeof(int16_t), 0, &addr, &size)<0))
				ERR("recvfrom:");
			portSend=ntohs(portSend);
			if(portSend == portSendOld)
				continue;

			if(DEBUG_OUT) printf("new port for sending is %d\n", portSend);
			if(DEBUG_OUT) printf("old port for sending was %d\n", portSendOld);

			if(socketSend > 0)
			{
				FD_CLR(socketSend, &rfdsDef);

				if(DEBUG_OUT) printf("old sending socket is closed\n");
				if(TEMP_FAILURE_RETRY(close(socketSend)<0))
					ERR("close");

				if(DEBUG_OUT) printf("old sender is killed\n");
				setSigHandler(SIG_IGN,SIGTERM);
				kill(0,SIGTERM);
				setSigHandler(SIG_DFL,SIGTERM);
			}

			if((socketSend = bind_inet_socket(portSend, SOCK_STREAM)) < 0)
				continue;

			addFlags(socketSend, O_NONBLOCK);
			fdmax = socketSend > socketReceive ? socketSend : socketReceive;

			FD_SET(socketSend, &rfdsDef);
		}

		if(FD_ISSET(socketSend, &rfds))
		{
			createSendingProcess(socketSend, &oldmask);
		}
	}

	kill(0, SIGINT);

	closeSocket(socketSend);
	closeSocket(socketReceive);
}

void usage()
{
	fprintf(stderr, "USAGE: only without any arguments\n");
	exitWithError("usage");
}

int main(int argc, char** argv)
{
	static const int port_num = 2000;

	if(argc > 1)
		usage();

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
