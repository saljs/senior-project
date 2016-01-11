#include "inputs.h"
#include "database.h"

#include <stdio.h>
#include <readline/readline.h>
#include <readline/history.h>

#include <stdio.h>
#include <iostream>
#include "opencv2/core/core.hpp"
#include "opencv2/features2d/features2d.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/nonfree/features2d.hpp"

using namespace cv;

input getInput(int type, memList* database)
{
    if(type == 0)
    {
        char* textIn = readline("Text to input:");
        if(textIn == NULL)
        {
            //do something with this later
        }
        else
        {
            input newInput;
            newInput.data = textIn;
            newInput.dataSize = strlen(textIn);
            linkInput(&newInput, type, database);
            return newInput;
        }

    }
    else if(type == 1)
    {
        char* imagePath = readline("Path to image input");
        if(imagePath == NULL)
        {
            //do something later with this
        }
        else
        {
            input newInput;
            Mat image = imread(imagePath, CV_LOAD_IMAGE_COLOR);
            newInput.data = malloc(sizeof(int)*3 + sizeof(size_t) + image.elemSize()*image.rows*image.cols);
            int ImgType = image.type();
            memmove(newInput.data, &image.rows, sizeof(int));
            memmove(newInput.data+sizeof(int), &image.cols, sizeof(int));
            memmove(newInput.data+sizeof(int)*2, &ImgType, sizeof(int));
            memmove(newInput.data+sizeof(int)*3, image.step.p, sizeof(size_t));
            memmove(newInput.data+sizeof(int)*3 + sizeof(size_t), image.data, image.elemSize()*image.rows*image.cols);
            newInput.dataSize = sizeof(int)*3 + sizeof(size_t) + image.elemSize()*image.rows*image.cols;
            linkInput(&newInput, type, database);
            return newInput;
        }
    }
