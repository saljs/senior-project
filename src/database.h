#include <sys/stat.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include "inputs.h"
#include "outputs.h"

#ifndef DATABASE_H_
#define DATABASE_H_

#define MAGIC_NUM 0xDEADF113
#define INPUT 0
#define OUTPUT 1

typedef struct input input;
typedef struct action action;
struct input
{
    char* data;
    size_t dataSize;
    long int link;
    float confidence;
    input* next;
};
struct action
{
    char* data;
    size_t dataSize;
    long int link;
    float confidence;
    action* next;
};
typedef struct 
{
    long uuid;
    input* inputs[NUMINPUTS];
    action* actions[NUMOUTPUTS];
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
    int type;
    int index;
} magic;

memList* newMemory(memList* start);
void addInput(memList* current, input* toAdd, int index);
void addAction(memList* current, action* toAdd, int index);
memList* loadDatabase(const char* filename);
int saveDatabase(const char* filename, memList* start);
#endif
