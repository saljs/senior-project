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
typedef struct memory memory;
typedef struct memory
{
    long int uuid;
    input* inputs[NUMINPUTS];
    memory* next;
} memory;
typedef struct magic
{
    unsigned int magicBits;
    int index;
} magic;

memory* newMemory(memory* start);
void addInput(memory* current, input* toAdd, int index);
memory* loadDatabase(const char* basedir);
int saveDatabase(const char* basedir, memory* start);
void disassemble(memory* start);
memory* AddtoMem(input* newInput, int index, memory* database, bool* newTrigger);
void linkInput(input* pattern, int type, memory* database);
memory* compile(input* pattern, memory* dataset, memory* list, int levels);

#endif
