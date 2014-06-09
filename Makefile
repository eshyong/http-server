CC=g++
CFLAGS=-c -g -std=c++0x
VERBOSE=-v

all: main.o server.o
	$(CC) server.o main.o -o grizzly

clean:
	rm -rf grizzly *.o *.dSYM

main.o: main.cc
	$(CC) $(CFLAGS) main.cc

server.o: server.cc
	$(CC) $(CFLAGS) server.cc


