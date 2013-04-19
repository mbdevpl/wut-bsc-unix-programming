all: fifos

fifos: mbdev_unix.o fifos.o
	gcc -Wall -o fifos fifos.o mbdev_unix.o

mbdev_unix.o: mbdev_unix.c mbdev_unix.h
	gcc -c mbdev_unix.c

fifos.o: fifos.c
	gcc -c fifos.c

.PHONY: clean

clean:
	rm fifos mbdev_unix.o fifos.o
