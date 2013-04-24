# unix_lab3_tut

all: server clientLocal clientTCP clientUDP

server: arithmetic_server.c
	gcc -Wall -o server arithmetic_server.c

clientLocal: arithmetic_clientLocal.c
	gcc -Wall -o clientLocal arithmetic_clientLocal.c

clientTCP: arithmetic_clientTCP.c
	gcc -Wall -o clientTCP arithmetic_clientTCP.c

clientUDP: arithmetic_clientUDP.c
	gcc -Wall -o clientUDP arithmetic_clientUDP.c

.PHONY: clean

clean:
	rm server clientLocal clientTCP clientUDP
