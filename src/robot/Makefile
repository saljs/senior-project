CC=g++
CFLAGS=$(shell pkg-config --cflags opencv) -lm -fpermissive --debug
LDFLAGS=$(shell pkg-config --libs opencv) -lpthread
WPIFLAG=-lwiringPi

all: robot

robot: main.o database.o inputs.o
	$(CC) $(CFLAGS) $(LDFLAGS) $(WPIFLAG) database.o inputs.o main.o -o robot

main.o: main.cpp
	$(CC) $(CFLAGS) $(LDFLAGS) $(WPIFLAG) -c main.cpp

database.o: database.c
	$(CC) $(CFLAGS) $(LDFLAGS) -c database.c
	
inputs.o: inputs.cpp
	$(CC) $(CFLAGS) $(LDFLAGS) $(WPIFLAG) -c inputs.cpp

cameratest: cameratest.cpp
	$(CC) $(CFLAGS) $(LDFLAGS) cameratest.cpp -o cameratest

clean:
	rm *.o robot cameratest
