CC = gcc
CFLAGS = -Wall

all: server client

server: main.c
	$(CC) $(CFLAGS) -o server main.c

client: client.c
	$(CC) $(CFLAGS) -o client client.c

clean:
	rm -f server client