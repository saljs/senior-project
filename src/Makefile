CC=g++
CFLAGS=$(shell pkg-config --cflags opencv) -lm -fpermissive
LDFLAGS=$(shell pkg-config --libs opencv) -lreadline -lcurses

all: demo

demo: main.o database.o inputs.o
	$(CC) $(CFLAGS) $(LDFLAGS) database.o inputs.o main.o -o demo

main.o: main.cpp
	$(CC) $(CFLAGS) $(LDFLAGS) -c main.cpp

database.o: database.c
	$(CC) $(CFLAGS) $(LDFLAGS) -c database.c
	
inputs.o: inputs.cpp
	$(CC) $(CFLAGS) $(LDFLAGS) -c inputs.cpp

clean:
	rm *.o demo
