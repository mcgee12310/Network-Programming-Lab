CC = gcc
CFLAGS = -Wall

all: server client

server: TCP_Server/server.o
	$(CC) $(CFLAGS) -o server TCP_Server/server.o

client: TCP_Client/client.o
	$(CC) $(CFLAGS) -o client TCP_Client/client.o

TCP_Server/server.o: TCP_Server/server.c
	$(CC) $(CFLAGS) -ITCP_Server -c TCP_Server/server.c -o TCP_Server/server.o

TCP_Client/client.o: TCP_Client/client.c
	$(CC) $(CFLAGS) -ITCP_Client -c TCP_Client/client.c -o TCP_Client/client.o

clean:
	rm -f TCP_Server/*.o TCP_Client/*.o server client
