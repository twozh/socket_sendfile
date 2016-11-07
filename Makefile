all: server.o client.o
	cc -o server server.o
	cc -o client client.o -lpthread

server.o: server.c
	cc -c server.c

client.o: client.c
	cc -c client.c

clean:
	rm client server

