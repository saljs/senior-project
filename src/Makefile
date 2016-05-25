CC=g++
CFLAGS=$(shell pkg-config --cflags opencv) -lm -fpermissive --debug
LDFLAGS=$(shell pkg-config --libs opencv) -lcurses -lpthread
FANNFLAGS=-ggdb -DDEBUG -Wall -ansi -IFANN/src/ -IFANN/src/include/ FANN/src/floatfann.c
MPIFLAGS=-pthread -Wl,-rpath -Wl,/usr/lib/openmpi -Wl,--enable-new-dtags -L/usr/lib/openmpi -lmpi_cxx -lmpi

all: server

server:main.o database.o inputs.o p_database.o
	$(CC) $(CFLAGS) $(LDFLAGS) $(FANNFLAGS) $(MPIFLAGS) p_database.o database.o inputs.o main.o -o server

main.o: main.cpp
	$(CC) $(CFLAGS) $(LDFLAGS) -c main.cpp

database.o: database.c
	$(CC) $(CFLAGS) $(LDFLAGS) -c database.c
	
p_database.o: p_database.c
	$(CC) $(CFLAGS) $(LDFLAGS) $(MPIFLAGS) -c p_database.c
	
inputs.o: inputs.cpp
	$(CC) $(CFLAGS) $(LDFLAGS) -c inputs.cpp
	
trainer:trainer.c
	$(CC) $(CFLAGS) $(LDFLAGS) $(FANNFLAGS) trainer.c -o trainer

clean:
	rm *.o server trainer
