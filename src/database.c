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

memList* newMemory(memList* start)
{
    memory* newMem = malloc(sizeof(memory));
    if(newMem == NULL)
    {
        return NULL;
    }
    if(start != NULL)
    {
        newMem->uuid = start->mem->uuid + 1;
    }
    else
    {
        newMem->uuid = 0;
    }
    for(int i = 0; i < NUMINPUTS; i++)
    {
        newMem->inputs[i] = NULL;
    }
    memList* newStart = malloc(sizeof(memList));
    if(newStart == NULL)
    {
        return NULL;
    }
    newStart->mem = newMem;
    newStart->next = start;
    return newStart;
}

void addInput(memory* current, input* toAdd, int index)
{
    toAdd->next = current->inputs[index];
    current->inputs[index] = toAdd;
}

memList* loadDatabase(const char* basedir)
{
    memList* last = NULL;
    memList* next;

    DIR* base = opendir(basedir);
    if(base == NULL)
    {
        return newMemory(NULL);
    }
    struct dirent* uuid_dir;
    while((uuid_dir = readdir(base)))
    {
        if(strcmp(uuid_dir->d_name, ".") == 0)
        {
            continue;
        }
        if(strcmp(uuid_dir->d_name, "..") == 0)
        {
            continue;
        }
        if(uuid_dir->d_type != DT_DIR)
        {
            continue;
        }
        memory* newMem = malloc(sizeof(memory));
        if(newMem == NULL)
        {
            return NULL;
        }
        newMem->uuid = atol(uuid_dir->d_name);
        char* filename = malloc(1000);
        if(filename == NULL)
        {
            return NULL;
        }
        sprintf(filename, "%s/%s", basedir, uuid_dir->d_name);
        DIR* memdir = opendir(filename);
        if(memdir == NULL)
        {
            return NULL;
        }
        free(filename);
        struct dirent* memfiles;
        //loop through save files for inputs
        while((memfiles = readdir(memdir)))
        {
            if(strcmp(memfiles->d_name, ".") == 0)
            {
                continue;
            }
            if(strcmp(memfiles->d_name, "..") == 0)
            {
                continue;
            }
            FILE* savefile;
            char* savename = malloc(1000);
            if(savename == NULL)
            {
                return NULL;
            }
            sprintf(savename, "%s/%s/%s", basedir, uuid_dir->d_name, memfiles->d_name);
            savefile = fopen(savename, "r");
            if(savefile == NULL)
            {
                continue;
            }
            free(savename);
            //parse save file
            magic magicBuff;
            if(fread(&magicBuff, sizeof(magic), 1, savefile) > 0)
            {
                if(magicBuff.magicBits != MAGIC_NUM)
                {
                    continue;
                }
                else 
                {
                    input* newInput = malloc(sizeof(input));
                    if(newInput == NULL)
                    {
                        return NULL;
                    }
                    input* lastInput = NULL;
                    newMem->inputs[magicBuff.index] = newInput;
                    //loop through to rebuild linked list
                    while(fread(&newInput->link, sizeof(long int), 1, savefile) > 0)
                    {
                        if(lastInput != NULL)
                        {
                            lastInput->next = newInput;
                        }
                        if(fread(&newInput->confidence, sizeof(float), 1, savefile) < 1)
                        {
                            return NULL;
                        }
                        if(fread(&newInput->dataSize, sizeof(size_t), 1, savefile) < 1)
                        {
                            return NULL;
                        }
                        newInput->data = malloc(newInput->dataSize);
                        if(newInput->data == NULL)
                        {
                            return NULL;
                        }
                        if(fread(newInput->data, newInput->dataSize, 1, savefile) < 1)
                        {
                            return NULL;
                        }

                        lastInput = newInput;
                        newInput = malloc(sizeof(input));
                        if(newInput == NULL)
                        {
                            return NULL;
                        }
                        lastInput->next = NULL;

                    }
                    newInput->next = NULL;
                }
            }
            fclose(savefile);
        }
        closedir(memdir);
        next = malloc(sizeof(memList));
        if(next == NULL)
        {
            return NULL;
        }
        next->mem = newMem;
        next->next = last;
        last = next;
    }
    closedir(base);
    return next;
}

int saveDatabase(const char* basedir, memList* start)
{
    memList* parser = start;
    mkdir(basedir, 0700);
    while(parser != NULL)
    {
        char* dirname = malloc(1000);
        if(dirname == NULL)
        {
            return -1;
        }
        sprintf(dirname, "%s/%d", basedir, parser->mem->uuid);
        mkdir(dirname, 0700);
        free(dirname);
        for(int i = 0; i < NUMINPUTS; i++)
        {
            char* filename = malloc(1000);
            if(filename == NULL)
            {
                return -2;
            }
            sprintf(filename, "%s/%d/I%d", basedir, parser->mem->uuid, i);
            FILE* inputDump = fopen(filename, "w");
            if(inputDump == NULL)
            {
                return -2;
            }
            magic inputOps;
            inputOps.index = i;
            inputOps.magicBits = MAGIC_NUM;
            if(fwrite(&inputOps, sizeof(magic), 1, inputDump) < 1)
            {
                return -2;
            }
            
            input* nextInput = parser->mem->inputs[i];
            while(nextInput != NULL)
            {
                if(fwrite(&nextInput->link, sizeof(long int), 1, inputDump) < 1)
                {
                    return -2;
                }
                if(fwrite(&nextInput->confidence, sizeof(float), 1, inputDump) < 1)
                {
                    return -2;
                }
                if(fwrite(&nextInput->dataSize, sizeof(size_t), 1, inputDump) < 1)
                {
                    return -2;
                }
                if(fwrite(nextInput->data, nextInput->dataSize, 1, inputDump) < 1)
                {
                    return -2;
                }
                input* tmp = nextInput->next;
                nextInput = tmp;
            }
        }
        memList* tmp = parser->next;
        parser = tmp;
    }
}

void disassemble(memList* start)
{
    memList* next = start;
    memList* last = NULL;
    while(next != NULL)
    {
        for(int i = 0; i < NUMINPUTS; i++)
        {
            input* nextInput = next->mem->inputs[i];
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
        memList* tmp = next->next;
        next = tmp;
        free(last->mem);
        free(last);
    }
}

void linkInput(input* pattern, int type, memList* database)
{
    float cost = 0, lastCost = 0, simConf = 0;
    memList* next = database;
    long int steps = 0, mostSim = -1;
    do
    {
        float matchprob = 0;
        input* currInput = next->mem->inputs[type];
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
            mostSim = next->mem->uuid;
            simConf = matchprob;
        }
        lastCost = cost;
        steps++;
        cost = steps / matchprob;
        if(matchprob > BRANCH_LIMIT)
        {
            memList* loop = next;
            while(loop->mem->uuid > tmpSim->link)
            {
                memList* tmp = loop->next;
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
            memList* tmp = next->next;
            next = tmp;
        }
    } while(cost * STOP_LIMIT > lastCost && next != NULL);
    pattern->link = mostSim;
    pattern->confidence = simConf;
    return;
}

memList* AddtoMem(input* newInput, int index, memList* database, bool* newTrigger)
{
    int count = 0;
    float mean = 0.0, sum = 0.0;
    input* next = database->mem->inputs[index];
    while(next != NULL)
    {
        sum += compareInputs(newInput, next, index);
        count++;
        input* tmp = next->next;
        next = tmp;
    }
    mean = sum / count;
    memList* updated;
    if(mean < SPLIT_LIMIT || isnan(mean))
    {
        if(newTrigger != NULL)
        {
            *newTrigger = true;
        }
        addInput(database->mem, newInput, index);
        updated = database;
    }
    else
    {
       if(newTrigger != NULL)
       {
           *newTrigger = true;
       }
       memList* newStart = newMemory(database);
       addInput(newStart->mem, newInput, index);
       updated = newStart;
    }
    return updated;
}


