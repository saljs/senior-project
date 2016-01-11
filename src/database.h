#include <sys/stat.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

#include "inputs.h"

#ifndef DATABASE_H_
#define DATABASE_H_

#define MAGIC_NUM 0xDEADF113
#define INPUT 0
#define OUTPUT 1

typedef struct input;
struct input
{
    char* data;
    size_t dataSize;
    long int link;
    float confidence;
    input* next;
};
typedef struct 
{
    long int uuid;
    input* inputs[NUMINPUTS];
} memory;
typedef struct memList;
struct memList
{
    memory* mem;
    memList* next;
};
typedef struct
{
    unsigned int magicBits;
    int index;
} magic;

memList* newMemory(memList* start);
void addInput(memory* current, input* toAdd, int index);
memList* loadDatabase(const char* basedir);
int saveDatabase(const char* basedir, memList* start);
void disassemble(memList* start);
memory* AddtoMem(memory* current, input* newInput, int index, memList* database, memory* newTrigger);
void linkInput(input* pattern, int type, memList* database);

#endif
