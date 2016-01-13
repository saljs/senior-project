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

int main(int argc, char* argv[])
{
    memList* database;
    if(argc < 1)
    {
        database = newMemory(NULL);
    }
    else
    {
        database = loadDatabase(argv[1]);
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
            memList* loop = database;
            database = AddtoMem(image, 1, database, NULL);
            while(loop->mem->uuid > image->link)
            {
                memList* tmp = loop->next;
                loop = tmp;
            }
            input* outLoop = loop->mem->inputs[0];
            while(outLoop != NULL)
            {
                printf("%d\n", outLoop->data);
                input* tmp = outLoop->next;
                outLoop = tmp;
            }
        }
        else if(text != NULL && image == NULL)
        {
            //outputting image based on text
            memList* loop = database;
            database = AddtoMem(text, 0, database, NULL);
            while(loop->mem->uuid > text->link)
            {
                memList* tmp = loop->next;
                loop = tmp;
            }
            input* outLoop = loop->mem->inputs[1];
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
        }
        else if(text != NULL && image != NULL)
        {
            //save both inputs and move on
            database = AddtoMem(text, 0, database, NULL);
            database = AddtoMem(image, 1, database, NULL);
        }

        char cont;
        printf("Continue? y/n : ");
        scanf("%c", &cont);
        if(cont == 'n')
        {
            break;
        }
    }
    if(saveDatabase(argv[1], database) != 0)
    {
        error("error saving the database!\n");
    }         
    disassemble(database);
    return 0;
}
