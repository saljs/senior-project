CC=g++
CFLAGS=$(shell pkg-config --cflags opencv) -lm -fpermissive --debug
LDFLAGS=$(shell pkg-config --libs opencv) -lreadline -lcurses -lpthread
FANNFLAGS=-ggdb -DDEBUG -Wall -ansi -IFANN/src/ -IFANN/src/include/ FANN/src/floatfann.c

all: server

server:main.o database.o inputs.o
	$(CC) $(CFLAGS) $(LDFLAGS) $(FANNFLAGS) database.o inputs.o main.o -o server

main.o: main.cpp
	$(CC) $(CFLAGS) $(LDFLAGS) -c main.cpp

database.o: database.c
	$(CC) $(CFLAGS) $(LDFLAGS) -c database.c
	
inputs.o: inputs.cpp
	$(CC) $(CFLAGS) $(LDFLAGS) -c inputs.cpp

clean:
	rm *.o server
