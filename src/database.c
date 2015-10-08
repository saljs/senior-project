#include "database.h"

memList* newMemory(memList* start)
{
    memory* newMem = malloc(sizeof(memory));
    newMem->uuid = start->mem->uuid + 1;
    for(int i = 0; i < NUMINPUTS; i++)
    {
        newMem->inputs[i] = NULL;
    }
    for(int i = 0; i < NUMOUTPUTS; i++)
    {
        newMem->actions = NULL;
    }
    memList* newStart = malloc(sizeof(memList));
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

memList* loadDatabase(const char* filename)
{
    FILE* database = fopen(filename, "r");
    memList* last = NULL;
    memList* next;
    memory* newMem = malloc(sizeof(memory));
    while(fread(newMem, sizeof(memory), 1, database) > 0)
    {
        next = malloc(sizeof(memList));
        next->mem = newMem;
        next->next = last;
        last = next;
        newMem = malloc(sizeof(memory));
    }
    fclose(database);
    return next;
}

int saveDatabase(const char* filename, memList* start)
{
    FILE* database = fopen(filename, "w");
    memList* parser = start;
    while(parser != NULL)
    {
        if(fwrite(parser->mem, sizeof(memory), 1, database) == 0)
        {
            return -1;
        }
        parser = parser->next;
    }
    fclose(database);
    return 0;
}
