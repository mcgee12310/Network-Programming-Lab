CC = gcc
CFLAGS = -Wall

all: server client

server: UDP_Server/server.o UDP_Server/resolver.o
	$(CC) $(CFLAGS) -o server UDP_Server/server.o UDP_Server/resolver.o

client: UDP_Client/client.o
	$(CC) $(CFLAGS) -o client UDP_Client/client.o

UDP_Server/server.o: UDP_Server/server.c UDP_Server/resolver.h
	$(CC) $(CFLAGS) -IUDP_Server -c UDP_Server/server.c -o UDP_Server/server.o

UDP_Server/resolver.o: UDP_Server/resolver.c UDP_Server/resolver.h
	$(CC) $(CFLAGS) -IUDP_Server -c UDP_Server/resolver.c -o UDP_Server/resolver.o

UDP_Client/client.o: UDP_Client/client.c
	$(CC) $(CFLAGS) -IUDP_Client -c UDP_Client/client.c -o UDP_Client/client.o

clean:
	rm -f UDP_Server/*.o UDP_Client/*.o server client
