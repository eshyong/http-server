CPPC=g++
CC=gcc
CFLAGS=-c -g -Wall
STD=-std=c++0x
VERBOSE=-v

all: main.o server.o ph7.o
	$(CPPC) server.o ph7.o main.o -o grizzly

clean:
	rm -rf grizzly *.o *.dSYM

main.o: main.cc
	$(CPPC) $(CFLAGS) $(STD) main.cc

server.o: server.cc
	$(CPPC) $(CFLAGS) $(STD) server.cc

ph7.o: PH7/ph7.c
	$(CC) $(CFLAGS) PH7/ph7.c
