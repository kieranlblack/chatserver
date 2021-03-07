default: clean server client

chatserver.o: chatserver.c chatserver.h defs.h
	gcc -c chatserver.c -g -o chatserver.o

server: chatserver.o
	gcc chatserver.o -o chatserver

chatclient.o: chatclient.c chatclient.h defs.h
	gcc -c chatclient.c -g -o chatclient.o

client: chatclient.o
	gcc chatclient.o -pthread -o chatclient

clean:
	-rm -f chatserver.o
	-rm -f chatclient.o
	-rm -f chatserver
	-rm -f chatclient
