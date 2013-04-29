# unix_lab4

all: server
#client

mbdev_unix.o: mbdev_unix.c mbdev_unix.h
	gcc -Wall -c mbdev_unix.c

lab4server.o: lab4server.c
	gcc -Wall -c lab4server.c

#lab4client.o: lab4client.c
#	gcc -Wall -c lab4client.c

server: mbdev_unix.o lab4server.o
	gcc -Wall -o server mbdev_unix.o lab4server.o -lpthread

#client: mbdev_unix.o lab4client.o
#	gcc -Wall -o client mbdev_unix.o lab4client.o

.PHONY: clean

clean:
	rm server mbdev_unix.o autoportserver.o
# client autoportclient.o
