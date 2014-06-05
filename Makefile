CC=g++
CFLAGS=-c -g -std=c++0x
VERBOSE=-v

all: server.o
	$(CC) server.o -o server

clean:
	rm -rf server *.o *.dSYM

server.o: server.cc
	$(CC) $(CFLAGS) server.cc


