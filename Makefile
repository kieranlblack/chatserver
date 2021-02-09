default: clean server client

server.o: server.c server.h defs.h
	gcc -c server.c -g -o server.o

server: server.o
	gcc server.o -o server

client.o: client.c client.h defs.h
	gcc -c client.c -g -o client.o

client: client.o
	gcc client.o -lpthread -o client

clean:
	-rm -f server.o
	-rm -f client.o
	-rm -f server
	-rm -f client
