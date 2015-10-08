
#include "inputs.h"
#include "outputs.h"

#ifndef DATABASE_H_
#define DATABASE_H_

typedef struct input input;
typedef struct action action;
struct input
{
    char* data;
    long int link;
    float confidence;
    input* next;
};
struct action
{
    char* data;
    long int link;
    float confidence;
    action* next;
};
typedef struct 
{
    long int uuid;
    input* inputs[NUMINPUTS];
    action* actions[NUMOUTPUTS];
} memory;
typedef struct memList;
struct memList
{
    memory* mem;
    memList* next;
};
memList* newMemory(memList* start);
void addInput(memList* current, input* toAdd, int index);
void addAction(memList* current, action* toAdd, int index);
memList* loadDatabase(const char* filename);
int saveDatabase(const char* filename, memList* start);
#endif
