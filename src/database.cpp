#include "database.h"

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
    for(int i = 0; i < NUMOUTPUTS; i++)
    {
        newMem->actions[i] = NULL;
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

void addInput(memList* current, input* toAdd, int index)
{
    toAdd->next = current->inputs[index];
    current->inputs[index] = toAdd;
}
void addAction(memList* current, action* toAdd, int index)
{
    toAdd->next = current->inputs[index];
    current->actions[index] = toAdd;
}

memList* loadDatabase(const char* basedir)
{
    memList* last = NULL;
    memList* next;

    DIR* base = opendir(basedir);
    if(base == NULL)
    {
        return = newMemory(NULL);
    }
    struct dirent* uuid_dir;
    while((uuid_dir = readdir(base)))
    {
        if(strcmp(uuid_dir->d_name, "."))
        {
            continue;
        }
        if(strcmp(uuid_dir->d_name, ".."))
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
        sprtinf(filename, "%s/%s", basedir, uuid_dir->d_name);
        DIR* memdir = opendir(filename);
        if(memdir == NULL)
        {
            return NULL;
        }
        free(filename);
        struct dirent* memfiles;
        //loop through save files for inputs/action
        while((memfiles = readdir(memdir)))
        {
            if(strcmp(memfiles->d_name, "."))
            {
                continue;
            }
            if(strcmp(memfiles->d_name, ".."))
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
                else if(magicBuff.type == INPUT)
                {
                    input* newInput = malloc(sizeof(input));
                    if(newInput == NULL)
                    {
                        return NULL
                    }
                    input* lastInput = NULL;
                    newMem->inputs[magicBuff.index] = newInput;
                    //loop through to rebuild linked list
                    while(fread(&newInput->link, sizeof(long int), 1, savefile) == sizeof(long int))
                    {
                        lastInput = newInput;
                        newInput = malloc(sizeof(input));
                        if(newInput == NULL)
                        {
                            return NULL;
                        }
                        lastInput->next = newInput;
                        if(fread(&newInput->link, sizeof(long int), 1, savefile) < sizeof(long int))
                        {
                            return NULL;
                        }
                        if(fread(&newInput->confidence, sizeof(float), 1, savefile) < sizeof(float))
                        {
                            return NULL;
                        }
                        if(fread(&newInput->dataSize, sizeof(size_t), 1, savefile) < sizeof(size_t))
                        {
                            return NULL;
                        }
                        newInput->data = malloc(newInput->dataSize);
                        if(newInput->data == NULL)
                        {
                            return NULL;
                        }
                        if(fread(newInput->data, newInpu->dataSize, 1, savefile) < newInput->dataSize)
                        {
                            return NULL;
                        }

                        lastInput = newInput;
                        newInput = malloc(sizeof(input));
                        if(newInput == NULL)
                        {
                            return NULL;
                        }
                        lastInput->next = newInput;

                    }
                    newInput->next = NULL;
                }
                else if(magicBuff.type == OUTPUT)
                {
                    action* newAction = malloc(sizeof(action));
                    if(newAction == NULL)
                    {
                        return NULL;
                    }
                    action* lastAction = NULL;
                    newMem->actions[magicBuff.index] = newAction;
                    //loop through to rebuild linked list
                    while(fread(&newAction->link, sizeof(long int), 1, savefile) == sizeof(long int))
                    {
                        lastAction = newAction;
                        newAction = malloc(sizeof(input));
                        if(newAction == NULL)
                        {
                            return NULL;
                        }
                        lastAction->next = newAction;
                        if(fread(&newAction->link, sizeof(long int), 1, savefile) < sizeof(long int))
                        {
                            return NULL;
                        }
                        if(fread(&newAction->confidence, sizeof(float), 1, savefile) < sizeof(float))
                        {
                            return NULL;
                        }
                        if(fread(&newAction->dataSize, sizeof(size_t), 1, savefile) < sizeof(size_t))
                        {
                            return NULL;
                        }
                        newAction->data = malloc(newAction->dataSize);
                        if(newAction->data == NULL)
                        {
                            return NULL;
                        }
                        if(fread(newAction->data, newInpu->dataSize, 1, savefile) < newAction->dataSize)
                        {
                            return NULL;
                        }

                        lastAction = newAction;
                        newAction = malloc(sizeof(input));
                        if(newAction == NULL)
                        {
                            return NULL;
                        }
                        lastAction->next = newAction;

                    }
                    newInput->next = NULL;
                }
            }
            fclose(savefile);
        }
        closedir(memdir);
        next = malloc(sizeof(memlist));
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

    if(!mkdir(basedir, 0700))
    {
        if(errno != EEXIST)
        {
            return -2;
        }
    }
    while(parser != NULL)
    {
        char* dirname = malloc(1000);
        if(dirname == NULL)
        {
            return -1;
        }
        sprintf(dirname, "%s/%s", basedir, parser->mem->uuid);
        if(!mkdir(dirname, 0700))
        {
            if(errno != EEXIST)
            {
                return -2;
            }
        }
        free(dirname);
        for(int i = 0; i < NUMINPUTS; i++)
        {
            char* filename = malloc(1000);
            if(filename == NULL)
            {
                return NULL;
            }
            sprintf(filename, "%s/%s/I%d", basedir, parser->mem->uuid, i);
            FILE* inputDump = fopen(filename, "w") 
            if(inputDump == NULL)
            {
                return -2;
            }
            magic inputOps;
            inputOps.type = INPUT;
            inputOps.index = i;
            inputOps.magicBits = MAGIC_NUM;
            if(fwrite(&inputOps, sizeof(magic), 1, inputDump) < sizeof(magic))
            {
                return -2;
            }
            
            input* nextInput = parser->mem->inputs[i];
            while(nextInput != NULL)
            {
                if(fwrite(&nextInput->link, sizeof(long int), 1, inputDump) < sizeof(long int))
                {
                    return -2;
                }
                if(fwrite(&nextInput->confidence, sizeof(float), 1, inputDump) < sizeof(float))
                {
                    return -2;
                }
                if(fwrite(&nextInput->dataSize, sizeof(size_t), 1, inputDump) < sizeof(size_t))
                {
                    return -2;
                }
                if(fwrite(nextInput->data, nextInput->dataSize, 1, inputDump) < nextInput->dataSize)
                {
                    return -2;
                }
                input* tmp = nextInput->next;
                nextInput = tmp;
            }
        }
        for(int i = 0; i < NUMOUTPUTS; i++)
        {
            char* filename = malloc(1000);
            if(filename == NULL)
            {
                return NULL;
            }
            sprintf(filename, "%s/%s/O%d", basedir, parser->mem->uuid, i);
            FILE* actionDump = fopen(filename, "w") 
            if(actionDump == NULL)
            {
                return -2;
            }
            magic actionOps;
            actionOps.type = INPUT;
            actionOps.index = i;
            actionOps.magicBits = MAGIC_NUM;
            if(fwrite(&actionOps, sizeof(magic), 1, actionDump) < sizeof(magic))
            {
                return -2;
            }
            
            action* nextAction = parser->mem->actions[i];
            while(nextAction != NULL)
            {
                if(fwrite(&nextAction->link, sizeof(long int), 1, actionDump) < sizeof(long int))
                {
                    return -2;
                }
                if(fwrite(&nextAction->confidence, sizeof(float), 1, actionDump) < sizeof(float))
                {
                    return -2;
                }
                if(fwrite(&nextAction->dataSize, sizeof(size_t), 1, actionDump) < sizeof(size_t))
                {
                    return -2;
                }
                if(fwrite(nextAction->data, nextInput->dataSize, 1, actionDump) < nextAction->dataSize)
                {
                    return -2;
                }
                action* tmp = nextAction->next;
                nextAction = tmp;
            }
        }
        memList* tmp = parser->next;
        parser = tmp;
    }
}

void disassmeble(memList* start)
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
        for(int i = 0; i < NUMACTIONS; i++)
        {
            action* nextAction = next->mem->actions[i];
            action* lastAction = NULL;
            while(nextAction != NULL)
            {
                free(nextAction->data);
                lastAction = nextAction;
                action* tmp = nextAction->next;
                nextAction = tmp;
                free(lastAction);
            }
        }
        last = next;
        memList* tmp = next->next;
        next = tmp;
        free(last->mem);
        free(last);
    }
}

long int SearchDatabase(input pattern, memList* database)
{
    float cost = 0, lastCost = 0, simConf = 0;
    long int steps = 0, mostSim;
    do
    {
        
    } while(cost * STOP_LIMIT < lastCost);
