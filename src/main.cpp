#include "database.h"
#include "inputs.h"
#include "vars.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <iostream>
#include "opencv2/core/core.hpp"
#include "opencv2/highgui/highgui.hpp"

using namespace std;
using namespace cv;

void error(const char* message)
{
    fputs(message, stderr);
}
void visualizer(memory* database)
{
    memory* loop = database;
    while(loop != NULL)
    {
        printf("|-mem:%d---------|\n|inputs:        |\n", loop->uuid);
        for(int i = 0; i < NUMINPUTS; i++)
        {
            printf("|  input%d ", i);
            input* inLoop = loop->inputs[i];
            while(inLoop != NULL)
            {
                printf("[link:%d confidence:%f], ", inLoop->link, inLoop->confidence);
                input* inTmp = inLoop->next;
                inLoop = inTmp;
            }
            printf("\n");
        }
        printf("|---------------|\n   |\n   |\n   |\n");
        memory* tmp = loop->next;
        loop = tmp;
    }
    printf(" |----|\n |NULL|\n |----|\n");
}
int main(int argc, char* argv[])
{
    memory* database;
    if(argc < 2)
    {
        database = newMemory(NULL);
    }
    else
    {
        database = loadDatabase(argv[1]);
    }
    if(database == NULL)
    {
        error("problem with the database\n");
        return -1;
    }
    while(true)
    {
        input* text = getInput(0, database);
        input* image = getInput(1, database);
        if(text == NULL && image == NULL)
        {
            error("No inputs given!\n");
        }
        else if(text == NULL && image != NULL)
        {
            //outputting text based on image
            memory* composite = compile(image, database, NULL, 1);
            database = AddtoMem(image, 1, database, NULL);
            input* outLoop = composite->inputs[0];
            while(outLoop != NULL)
            {
                printf("%s\n", outLoop->data);
                input* tmp = outLoop->next;
                outLoop = tmp;
            }
            disassemble(composite);
        }
        else if(text != NULL && image == NULL)
        {
            //outputting image based on text
            memory* composite = compile(text, database, NULL, 1);
            database = AddtoMem(text, 0, database, NULL);
            input* outLoop = composite->inputs[1];
            while(outLoop != NULL)
            {
                int rows, cols, type;
                size_t step;
                void* data = malloc(outLoop->dataSize - sizeof(int)*3+sizeof(size_t));
                memmove(&rows, outLoop->data, sizeof(int));
                memmove(&cols, outLoop->data+sizeof(int), sizeof(int));
                memmove(&type, outLoop->data+sizeof(int)*2, sizeof(int));
                memmove(&step, outLoop->data+sizeof(int)*3, sizeof(size_t));
                memmove(data, outLoop->data+sizeof(int)*3+sizeof(size_t), outLoop->dataSize - sizeof(int)*3+sizeof(size_t));
                Mat img(rows, cols, type, data, step);
                imshow("Image", img);
                waitKey(0);
                input* tmp = outLoop->next;
                outLoop = tmp;
            }
            disassemble(composite);
        }
        else if(text != NULL && image != NULL)
        {
            //save both inputs and move on
            database = AddtoMem(text, 0, database, NULL);
            database = AddtoMem(image, 1, database, NULL);
        }

        char cont;
        //getchar();
        printf("Continue? y/n : ");
        scanf(" %c", &cont);
        if(cont == 'n')
        {
            break;
        }
    }
    if(argc < 2)
    {
        int save = saveDatabase("database", database); 
        if(save != 0)
        {
            error("error saving the database!\n");
            printf("Error code: %d\n", save);
        }         
    }
    else
    {
        int save = saveDatabase(argv[1], database);
        if(save != 0)
        {
            error("error saving the database!\n");
            printf("Error code: %d\n", save);
        }         
    }
    disassemble(database);
    return 0;
}
