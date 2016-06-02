/* database.c
 * This file all the database-specific functions that
 * remain constant across implemtations.
 */

#include "database.h"
#include "inputs.h"
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <errno.h>
#include <stdbool.h>
#include <math.h>

/* Returns a pointer to a sanitized new memory, or NULL on error. If you pass it a memory already part of a list it will append the list, otherwise if it's passed NULL it will start a new list. */
memory* newMemory(memory* start)
{
    memory* newMem = malloc(sizeof(memory));
    if(newMem == NULL)
    {
        return NULL;
    }
    if(start != NULL)
    {
        newMem->uuid = start->uuid + 1;
    }
    else
    {
        newMem->uuid = 0;
    }
    for(int i = 0; i < NUMINPUTS; i++)
    {
        newMem->inputs[i] = NULL;
    }
    newMem->next = start;
    return newMem;
}

/* Adds an input to the current memory passed to it. The index is the index of the input. */
void addInput(memory* current, input* toAdd, int index)
{
    toAdd->next = current->inputs[index];
    current->inputs[index] = toAdd;
}

/* Safely frees the memory list passed to it. */
void disassemble(memory* start)
{
    memory* next = start;
    memory* last = NULL;
    while(next != NULL)
    {
        for(int i = 0; i < NUMINPUTS; i++)
        {
            input* nextInput = next->inputs[i];
            input* lastInput = NULL;
            while(nextInput != NULL)
            {
                free(nextInput->data);
                lastInput = nextInput;
                input* tmp = nextInput->next;
                nextInput = tmp;
                free(lastInput);
            }
        }
        last = next;
        memory* tmp = next->next;
        next = tmp;
        free(last);
    }
}

/* Parses over the database and links the input pattern. The database arg is the database to be searched, and the type is the index of the input pattern. */
void linkInput(input* pattern, int type, memory* database)
{
    float cost = 0, lastCost = 0, simConf = 0;
    memory* next = database;
    long int steps = 0, mostSim = -1;
    do
    {
        float matchprob = 0;
        input* currInput = next->inputs[type];
        input* tmpSim;
        while(currInput != NULL) //iterate over all inputs in current memory
        {
            float similarity = compareInputs(pattern, currInput, type); //compare them with the pattern input
            if(similarity > matchprob) //if the input is the most similar of all inputs in the memory so far, save it
            {
                matchprob = similarity;
                tmpSim = currInput;
            }
            input* tmp = currInput->next;
            currInput = tmp;
        }
        if(matchprob > simConf) //if the current similar is the most similar so far, save it
        {
            mostSim = next->uuid;
            simConf = matchprob;
        }
        lastCost = cost;
        steps++;
        cost = steps / (matchprob + 1); //calculate algorithm cost
        if(matchprob > BRANCH_LIMIT) //if the current memory is more similar than the BRANCH_LIMIT, go to the linked memory instead of iterating linearly
        {
            memory* loop = next;
            while(loop->uuid > tmpSim->link)
            {
                memory* tmp = loop->next;
                loop = tmp;
                if(loop == NULL)
                {
                    break;
                }
            }
            next = loop;
        }
        else //iterate over memory list linearly
        {
            memory* tmp = next->next;
            next = tmp;
        }
    } while(cost * STOP_LIMIT > lastCost && next != NULL); //check stop conditions
    pattern->link = mostSim; //link input to the most similar one found
    pattern->confidence = simConf;
    return;
}

/* Adds a new input to the memory list, deciding whether to append to current memory or create a new one. If newTrigger is not NULL and points to a bool, it will be set to true if there is a split, and false otherwise. Returns the start of the database, possibly changed, or NULL on error. */
memory* AddtoMem(input* newInput, int index, memory* database, bool* newTrigger)
{
    int count = 0;
    float mean = 0.0, sum = 0.0;
    input* next = database->inputs[index];
    while(next != NULL)
    {
        sum += compareInputs(newInput, next, index);
        count++;
        input* tmp = next->next;
        next = tmp;
    }
    mean = sum / count; //calculate mean similarity to the inputs currently in the memory
    memory* updated;
    if(mean > SPLIT_LIMIT || isnan(mean)) //append the input to current memory
    {
        if(newTrigger != NULL)
        {
            *newTrigger = false;
        }
        addInput(database, newInput, index);
        updated = database;
    }
    else //split the input into a new memory
    {
        if(newTrigger != NULL)
        {
            *newTrigger = true;
        }
        memory* newStart = newMemory(database);
        if(newStart == NULL)
        {
            return NULL;
        }
        addInput(newStart, newInput, index);
        updated = newStart;
    }
    return updated;
}

/* Compiles a new memory list, int levels deep, based on what pattern is linked to. Dataset should point to the database, and list should point to a NULL memory pointer, that the composite will eventually be stored in. */
void compileMem(input* pattern, memory* dataset, memory** list, int levels)
{
    if(levels == 0)
    {
        return;
    }
    if(list == NULL)
    {
        return;
    }
    memory* dataList = newMemory(*list);
    *list = dataList;
    memory* loop = dataset;
    while(loop->uuid > pattern->link) //jump to the linked memory in pattern
    {
        memory* tmp = loop->next;
        loop = tmp;
        if(loop == NULL)
        {
            return NULL;
        }
    }
    for(int i = 0; i < NUMINPUTS; i++) //copy the input data into a new list
    {
        input* parser = loop->inputs[i];
        while(parser != NULL)
        {
            input* newInput = malloc(sizeof(input));
            void* newData = malloc(parser->dataSize);
            memmove(newData, parser->data, parser->dataSize);
            memmove(newInput, parser, sizeof(input));
            newInput->data = newData;
            addInput(dataList, newInput, i);
            input* tmp = parser->next;
            parser = tmp;
        }
    }
    for(int i = 0; i < NUMINPUTS; i++) //iterate over all inputs in the memory
    {
        input* parser = loop->inputs[i];
        while(parser != NULL)
        {
            compileMem(parser, loop, list, levels-1); //recurse until levels reaches 0
            input* tmp = parser->next;
            parser = tmp;
        }
    }
    return;
}

/* Loads the memory database from savefile and returns a pointer to it on success, or NULL on an error */
memory* loadDatabase(const char* savefile)
{
    //open save file
    FILE* saved = fopen(savefile, "r");
    if(saved == NULL)
    {
        return NULL;
    }
    //recurse over the save file, load magic data, create memories as needed, add inputs
    magic magicBuff;
    memory* next = newMemory(NULL);
    if(next == NULL)
    {
        return NULL;
    }
    memory* first = next;
    memory* last;
    //read first input
    if(fread(&magicBuff, sizeof(magic), 1, saved) < 1)
    {
        return NULL;
    }
    next->uuid = magicBuff.uuid;
    input* newInput = malloc(sizeof(input));
    if(newInput == NULL)
    {
        return NULL;
    }
    newInput->dataSize = magicBuff.dataSize;
    newInput->link = magicBuff.link;
    newInput->confidence = magicBuff.confidence;
    newInput->data = malloc(magicBuff.dataSize);
    if(fread(newInput->data, magicBuff.dataSize, 1, saved) < 1)
    {
        return NULL;
    }
    //append to memory
    next->inputs[magicBuff.index] = newInput;
    newInput->next = NULL;
    //read the rest
    while(fread(&magicBuff, sizeof(magic), 1, saved) > 0)
    {
        if(magicBuff.uuid != next->uuid)
        {
            last = next;
            next = newMemory(NULL);
            if(next == NULL)
            {
                return NULL;
            }
            next->uuid = magicBuff.uuid;
            next->next = NULL;
            last->next = next;
        }
        newInput = malloc(sizeof(input));
        if(newInput == NULL)
        {
            return NULL;
        }
        newInput->dataSize = magicBuff.dataSize;
        newInput->link = magicBuff.link;
        newInput->confidence = magicBuff.confidence;
        newInput->data = malloc(magicBuff.dataSize);
        if(fread(newInput->data, magicBuff.dataSize, 1, saved) < 1)
        {
            return NULL;
        }
        input* loop = next->inputs[magicBuff.index];
        if(loop == NULL)
        {
            next->inputs[magicBuff.index] = newInput;
        }
        else
        {
            while(loop->next != NULL)
            {
                input* tmp = loop->next;
                loop = tmp;
            }
            loop->next = newInput;
        }
        newInput->next = NULL;
    }
    fclose(saved);
    return first;
}

/* Saves the database to a savefile. Returns -1 on read errors, and -2 if there's an error opening the file. */
int saveDatabase(const char* savefile, memory* start)
{
    //open save file
    FILE* saveto = fopen(savefile, "w");
    if(saveto == NULL)
    {
        return -2;
    }
    magic magicBuff;
    memory* next = start;
    while(next != NULL)
    {
        for(int i = 0; i < NUMINPUTS; i++)
        {
            input* nextInput = next->inputs[i];
            while(nextInput != NULL)
            {
                magicBuff.uuid = next->uuid;
                magicBuff.index = i;
                magicBuff.dataSize = nextInput->dataSize;
                magicBuff.link = nextInput->link;
                magicBuff.confidence = nextInput->confidence;
                if(fwrite(&magicBuff, sizeof(magic), 1, saveto) < 1)
                {
                    return -1;
                }
                if(fwrite(nextInput->data, nextInput->dataSize, 1, saveto) < 1)
                {
                    return -1;
                }
                input* tmp = nextInput->next;
                nextInput = tmp;
            }
        }
        memory* tmp = next->next;
        next = tmp;
    }
    fclose(saveto);
    return 0;
}
