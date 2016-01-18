#include "inputs.h"

#include <stdio.h>
#include <string.h>
#include <readline/readline.h>
#include <readline/history.h>

#include <iostream>
#include "opencv2/core/core.hpp"
#include "opencv2/features2d/features2d.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/nonfree/features2d.hpp"
#include "opencv2/imgproc/imgproc.hpp"

using namespace std;
using namespace cv;

input* getInput(int type, memory* database)
{
    if(type == 0)
    {
        char* textIn = readline("Text to input:");
        if(textIn == NULL)
        {
            return NULL;
        }
        else
        {
            input* newInput = (input*)malloc(sizeof(input));
            newInput->data = textIn;
            newInput->dataSize = strlen(textIn);
            linkInput(newInput, type, database);
            return newInput;
        }

    }
    else if(type == 1)
    {
        char* imagePath = readline("Path to image input");
        if(imagePath == NULL)
        {
            return NULL;
        }
        else
        {
            input* newInput = (input*)malloc(sizeof(input));
            Mat image = imread(imagePath, CV_LOAD_IMAGE_COLOR);
            newInput->data = malloc(sizeof(int)*3 + sizeof(size_t) + image.elemSize()*image.rows*image.cols);
            int ImgType = image.type();
            memmove(newInput->data, &image.rows, sizeof(int));
            memmove(newInput->data+sizeof(int), &image.cols, sizeof(int));
            memmove(newInput->data+sizeof(int)*2, &ImgType, sizeof(int));
            memmove(newInput->data+sizeof(int)*3, image.step.p, sizeof(size_t));
            memmove(newInput->data+sizeof(int)*3 + sizeof(size_t), image.data, image.elemSize()*image.rows*image.cols);
            newInput->dataSize = sizeof(int)*3 + sizeof(size_t) + image.elemSize()*image.rows*image.cols;
            linkInput(newInput, type, database);
            return newInput;
        }
    }
    return NULL;
}

float compareInputs(input* input1, input* input2, int type)
{
    float similarity = 0;
    if(type == 0)
    {
        //compare the two strings for occurances of the same word
        int count = 0, size = 0;
        char* string1 = (char*)malloc(input1->dataSize);
        char* string2 = (char*)malloc(input2->dataSize);
        memmove(string1, input1->data, input1->dataSize);
        memmove(string2, input2->data, input2->dataSize);
        char* saveptr1;
        char* saveptr2;
        char* firstWord = strtok_r(string1, " ", &saveptr1);
        while(firstWord != NULL)
        {
            char* secondWord = strtok_r(string2, " ", &saveptr2);
            while(secondWord != NULL)
            {
                if(strcmp(firstWord, secondWord) == 0)
                {
                    count++;
                }
                secondWord = strtok_r(NULL, " ", &saveptr2);
            }
            firstWord = strtok_r(NULL, " ", &saveptr1);
            size++;
        }
        similarity = count / size;
        free(string1);
        free(string2);
    }
    else if(type == 1)
    {
        //compare two images using FLANN matching algorithm
        //rebuild Mat objects from inputs
        int rows, cols, type;
        size_t step;
        void* data1 = malloc(input1->dataSize - sizeof(int)*3+sizeof(size_t));
        void* data2 = malloc(input2->dataSize - sizeof(int)*3+sizeof(size_t));

        memmove(&rows, input1->data, sizeof(int));
        memmove(&cols, input1->data+sizeof(int), sizeof(int));
        memmove(&type, input1->data+sizeof(int)*2, sizeof(int));
        memmove(&step, input1->data+sizeof(int)*3, sizeof(size_t));
        memmove(data1, input1->data+sizeof(int)*3+sizeof(size_t), input1->dataSize - sizeof(int)*3+sizeof(size_t));
        Mat img_1(rows, cols, type, data1, step);

        memmove(&rows, input2->data, sizeof(int));
        memmove(&cols, input2->data+sizeof(int), sizeof(int));
        memmove(&type, input2->data+sizeof(int)*2, sizeof(int));
        memmove(&step, input2->data+sizeof(int)*3, sizeof(size_t));
        memmove(data2, input2->data+sizeof(int)*3+sizeof(size_t), input2->dataSize - sizeof(int)*3+sizeof(size_t));
        Mat img_2(rows, cols, type, data2, step);
        
        //calculate similarity using FLANN matching
        if(!img_1.data || !img_2.data)
        {
            return 0;
        }
        int minHessian = 400;
        SurfFeatureDetector detector(minHessian);
        std::vector<KeyPoint> keypoints_1, keypoints_2;
        detector.detect(img_1, keypoints_1);
        detector.detect(img_2, keypoints_2);
        SurfDescriptorExtractor extractor;
        Mat descriptors_1, descriptors_2;
        extractor.compute(img_1, keypoints_1, descriptors_1);
        extractor.compute(img_2, keypoints_2, descriptors_2);
        FlannBasedMatcher matcher;
        std::vector< DMatch > matches;
        matcher.match(descriptors_1, descriptors_2, matches);
        double max_dist = 0, min_dist = 100;
        for( int i = 0; i < descriptors_1.rows; i++ )
        { 
            double dist = matches[i].distance;
            if( dist < min_dist ) 
            {
                min_dist = dist;
            }
            if( dist > max_dist ) 
            {
                max_dist = dist;
            }
        }
        std::vector< DMatch > good_matches;
        for( int i = 0; i < descriptors_1.rows; i++ )
        { 
            if( matches[i].distance <= max(2*min_dist, 0.02) )
            { 
                good_matches.push_back( matches[i]); 
            }
        }
        double TotalDist = 0.0;
        for( int i = 0; i < (int)good_matches.size(); i++ )
        {
            TotalDist += good_matches[i].distance;
        }
        double avgdist = TotalDist / (int)good_matches.size();
        double FLANNtest = 1 - avgdist;
        
        //calculate similarity using histograms
        Mat hsv_1, hsv_2;
        cvtColor(img_1, hsv_1, COLOR_BGR2HSV);
        cvtColor(img_2, hsv_2, COLOR_BGR2HSV);
        int histSize[] = {50, 60};
        float h_ranges[] = { 0, 180 };
        float s_ranges[] = { 0, 256 };
        const float* ranges[] = { h_ranges, s_ranges };
        int channels[] = {0, 1};
        MatND hist_1, hist_2;
        calcHist(&hsv_1, 1, channels, Mat(), hist_1, 2, histSize, ranges, true, false);
        normalize(hist_1, hist_1, 0, 1, NORM_MINMAX, -1, Mat());
        calcHist(&hsv_2, 1, channels, Mat(), hist_2, 2, histSize, ranges, true, false);
        normalize(hist_2, hist_2, 0, 1, NORM_MINMAX, -1, Mat());
        double HISTtest = 1 - compareHist(hist_1, hist_2, 3);
        
        //average the results
        similarity = (FLANNtest + HISTtest)/2;
        free(data1);
        free(data2);
    }
    return similarity;
}
