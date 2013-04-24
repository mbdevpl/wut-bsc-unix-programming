# unix_lab3_fix

all: server client

mbdev_unix.o: mbdev_unix.c mbdev_unix.h
	gcc -Wall -c mbdev_unix.c

autoportserver.o: autoportserver.c
	gcc -Wall -c autoportserver.c

autoportclient.o: autoportclient.c
	gcc -Wall -c autoportclient.c

server: mbdev_unix.o autoportserver.o
	gcc -Wall -o server mbdev_unix.o autoportserver.o

client: mbdev_unix.o autoportclient.o
	gcc -Wall -o client mbdev_unix.o autoportclient.o

.PHONY: clean

clean:
	rm server client mbdev_unix.o autoportserver.o autoportclient.o
