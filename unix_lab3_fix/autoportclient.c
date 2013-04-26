#include "mbdev_unix.h"

#define SERVERPORT 2000

#define DEBUG_OUT 1

#define MSGSIZE 1000

int connect_socket(char *name, uint16_t port)
{
	struct sockaddr_in addr;
	int socket;
	int socketStatus;
	socklen_t size = sizeof(int);
	fd_set socketFds;

	socket = makeSocket(PF_INET, SOCK_STREAM);
	addr = make_address(name, port);

	if(connect(socket, (struct sockaddr*)&addr, sizeof(struct sockaddr_in)) < 0)
	{
		switch(errno)
		{
		case EINTR:
			{
				FD_ZERO(&socketFds);
				FD_SET(socket, &socketFds);
				if(TEMP_FAILURE_RETRY(select(socket + 1, NULL, &socketFds, NULL, NULL)) < 0)
					ERR("select");

				if(getsockopt(socket, SOL_SOCKET, SO_ERROR, &socketStatus, &size)<0)
					ERR("getsockopt");
				if(socketStatus)
					ERR("connect");
			} break;
		default: ERR("connect tcp socket");
		}
	}
	return socket;
}

void clientWork(char* ipAddress, int16_t port)
{
	int socketSend;
	char buf[MSGSIZE];
	int serverActive = 1;

	if(DEBUG_OUT) printf("connecting to %s:%d\n", ipAddress, ntohs(port));
	socketSend = connect_socket(ipAddress, port);
	memset((void*)buf, 0, MSGSIZE);
	while(fgets(buf, MSGSIZE, stdin) && serverActive){
		if(strlen(buf))
			buf[strlen(buf) - 1] = '\0';

		if(bulk_write(socketSend, buf, strlen(buf)) < 0)
		{
			switch(errno)
			{
			case EPIPE:
			{
				if(DEBUG_OUT) printf("EPIPE\n");
				serverActive = 0;
			} break;
			default: ERR("socket send");
			}
		}
	}
	if(DEBUG_OUT) printf("client exits\n");

	closeSocket(socketSend);
}

void usage()
{
	fprintf(stderr, "USAGE: IP port\n");
	exitWithError("usage");
}

int main(int argc, char** argv)
{
	int socket;
	struct sockaddr_in addr;
	int16_t port;

	if(argc != 3)
		usage();

	setSigHandler(SIG_IGN,SIGPIPE);
	socket = makeSocket(PF_INET, SOCK_DGRAM);
	addr = make_address(argv[1], SERVERPORT);

	port = (int16_t)atoi(argv[2]);
	port = htons(port);

	if(TEMP_FAILURE_RETRY(sendto(socket, (char *)&port, sizeof(int16_t), 0, (struct sockaddr*)&addr, sizeof(addr))) < 0)
		ERR("sendto:");

	if(DEBUG_OUT) printf("client sleeps for 1 second\n");
	milisleepFor(1000);

	clientWork(argv[1], port);

	closeSocket(socket);

	return EXIT_SUCCESS;
}
