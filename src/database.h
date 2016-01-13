#include <stdio.h>
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "vars.h"

#ifndef DATABASE_H_
#define DATABASE_H_

#define MAGIC_NUM 0xDEADF113
#define INPUT 0
#define OUTPUT 1

typedef struct input input;
typedef struct input
{
    void* data;
    size_t dataSize;
    long int link;
    float confidence;
    input* next;
};
typedef struct memory
{
    long int uuid;
    input* inputs[NUMINPUTS];
} memory;
typedef struct memList memList;
typedef struct memList
{
    memory* mem;
    memList* next;
};
typedef struct magic
{
    unsigned int magicBits;
    int index;
} magic;

memList* newMemory(memList* start);
void addInput(memory* current, input* toAdd, int index);
memList* loadDatabase(const char* basedir);
int saveDatabase(const char* basedir, memList* start);
void disassemble(memList* start);
memList* AddtoMem(input* newInput, int index, memList* database, bool* newTrigger);
void linkInput(input* pattern, int type, memList* database);

#endif
