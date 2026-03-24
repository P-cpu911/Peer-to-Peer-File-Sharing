CC = gcc
CFLAGS = -Wall -pthread
TARGETS = index_server peer testfiletransfer

all: $(TARGETS)

index_server: index_server.c
	$(CC) $(CFLAGS) index_server.c -o index_server

peer: peer.c
	$(CC) $(CFLAGS) peer.c -o peer

testfiletransfer: testfiletransfer.c
	$(CC) $(CFLAGS) testfiletransfer.c -o testfiletransfer

clean:
	rm -f $(TARGETS)
