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

void addInput(memory* current, input* toAdd, int index)
{
    toAdd->next = current->inputs[index];
    current->inputs[index] = toAdd;
}

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
        while(currInput != NULL)
        {
            float similarity = compareInputs(pattern, currInput, type);
            if(similarity > matchprob)
            {
                matchprob = similarity;
                tmpSim = currInput;
            }
            input* tmp = currInput->next;
            currInput = tmp;
        }
        if(matchprob > simConf)
        {
            mostSim = next->uuid;
            simConf = matchprob;
        }
        lastCost = cost;
        steps++;
        cost = steps / matchprob;
        if(matchprob > BRANCH_LIMIT)
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
        else
        {
            memory* tmp = next->next;
            next = tmp;
        }
    } while(cost * STOP_LIMIT > lastCost && next != NULL);
    pattern->link = mostSim;
    pattern->confidence = simConf;
    return;
}

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
    mean = sum / count;
    memory* updated;
    if(mean > SPLIT_LIMIT || isnan(mean))
    {
        if(newTrigger != NULL)
        {
            *newTrigger = true;
        }
        addInput(database, newInput, index);
        updated = database;
    }
    else
    {
       if(newTrigger != NULL)
       {
           *newTrigger = true;
       }
       memory* newStart = newMemory(database);
       addInput(newStart, newInput, index);
       updated = newStart;
    }
    return updated;
}

memory* compile(input* pattern, memory* dataset, memory* list, int levels)
{
    if(levels == 0)
    {
        return NULL;
    }
    memory* dataList = newMemory(list);
    
    memory* loop = dataset;
    while(loop->uuid > pattern->link)
    {
        memory* tmp = loop->next;
        loop = tmp;
        if(loop == NULL)
        {
            return NULL;
        }
    }
    for(int i = 0; i < NUMINPUTS; i++)
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
    for(int i = 0; i < NUMINPUTS; i++)
    {
        input* parser = loop->inputs[i];
        while(parser != NULL)
        {
            compile(parser, loop, dataList, levels-1);
            input* tmp = parser->next;
            parser = tmp;
        }
    }
    return dataList;
}

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
