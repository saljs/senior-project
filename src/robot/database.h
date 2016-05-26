/* database.h
 * This file contains the different datatypes and
 * function declarations for the library.
 * Descriptions of the functions are in database.c.
 */

#include <stdio.h>
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "vars.h"

#ifndef DATABASE_H_
#define DATABASE_H_

typedef struct input input;
typedef struct input  //input struct, stores the individual inputs in a linked list.
{
    void* data;       //actual input data, can be anything
    size_t dataSize;  //size of the input data
    long int link;    //link to the most similar memory, entered by linkInput function
    float confidence; //confidence of the link, added by linkInput function
    input* next;      //pointer to next input in the linked list
};
typedef struct memory memory;
typedef struct memory          //memory struct, stores input lists
{
    long int uuid;            //UUID for the memory, just counts up from 0
    input* inputs[NUMINPUTS]; //pointers to input lists
    memory* next;             //pointer to next memory in liked list
} memory;
typedef struct magic   //magic struct used for storing and reading info to files, all handled by loadDatabase and saveDatabase functions.
{
    long int uuid;
    int index;
    size_t dataSize;
    long int link;
    float confidence;
} magic;

memory* newMemory(memory* start);
void addInput(memory* current, input* toAdd, int index);
memory* loadDatabase(const char* basedir);
int saveDatabase(const char* basedir, memory* start);
void disassemble(memory* start);
memory* AddtoMem(input* newInput, int index, memory* database, bool* newTrigger);
void linkInput(input* pattern, int type, memory* database);
void compileMem(input* pattern, memory* dataset, memory** list, int levels);

#endif
