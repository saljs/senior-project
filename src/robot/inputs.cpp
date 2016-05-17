#include "inputs.h"

#include <pthread.h>
#include <stdbool.h>
#include <iostream>
#include "opencv2/core/core.hpp"
#include "opencv2/features2d/features2d.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/nonfree/features2d.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include <math.h>
#include "hardware.h"

using namespace std;
using namespace cv;

Mat cameraFrame;
bool closeThread = false;

void* capture(void* arg)
{
    VideoCapture camera;
    if(!camera.open(0))
    {
        pthread_exit(NULL);
    }
    while(!closeThread)
    {
        camera >> cameraFrame;
    }
    pthread_exit(NULL);
    return NULL;
}

void startCapture()
{
    pthread_t captureThread;
    pthread_create(&captureThread, NULL, capture, NULL);
}

void stopCapture()
{
    closeThread = true;
}

input* getInput(int type, memory* database)
{
    if(type == 0) //frame from camera
    {
        input* newInput = (input*)malloc(sizeof(input));
        Mat image = cameraFrame;
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
    else if(type == 1) //ultrasonic sensor
    {
        //Send trig pulse
        digitalWrite(TRIG, HIGH);
        delayMicroseconds(10);
        digitalWrite(TRIG, LOW);
        //Wait for echo start
        while(digitalRead(ECHO) == LOW);
        //Wait for echo end
        long startTime = micros();
        while(digitalRead(ECHO) == HIGH);
        long travelTime = micros() - startTime;
        //Get distance in cm
        input* newInput = (input*)malloc(sizeof(input));
        newInput->data = malloc(sizeof(float));
        *(float *)newInput->data = travelTime * 0.01715;
        newInput->dataSize = sizeof(float);
        linkInput(newInput, type, database);
        return newInput;
    }
    else if(type == 2)//left light sensor
    {
        input* newInput = (input*)malloc(sizeof(input));
        newInput->data = malloc(sizeof(float));
        *(float *)newInput->data = (float)ananlogRead(SOLAR_L) / 1024.0;
        newInput->dataSize = sizeof(float);
        linkInput(newInput, type, database);
        return newInput;
    }
    else if(type == 3)//right light sensor
    {
        input* newInput = (input*)malloc(sizeof(input));
        newInput->data = malloc(sizeof(float));
        *(float *)newInput->data = (float)ananlogRead(SOLAR_R) / 1024.0;
        newInput->dataSize = sizeof(float);
        linkInput(newInput, type, database);
        return newInput;
    }
    else if(type == 5)//top light sensor
    {
        input* newInput = (input*)malloc(sizeof(input));
        newInput->data = malloc(sizeof(float));
        *(float *)newInput->data = (float)ananlogRead(SOLAR_T) / 1024.0;
        newInput->dataSize = sizeof(float);
        linkInput(newInput, type, database);
        return newInput;
    }
    return NULL;
}

float compareInputs(input* input1, input* input2, int type)
{
    float similarity = 0;
    if(type == 0)
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
        double FLANNtest;
        int minHessian = 400;
        SurfFeatureDetector detector(minHessian);
        std::vector<KeyPoint> keypoints_1, keypoints_2;
        detector.detect(img_1, keypoints_1);
        detector.detect(img_2, keypoints_2);
        SurfDescriptorExtractor extractor;
        Mat descriptors_1, descriptors_2;
        extractor.compute(img_1, keypoints_1, descriptors_1);
        extractor.compute(img_2, keypoints_2, descriptors_2);
        if (descriptors_1.empty() || descriptors_2.empty())
        {
            //no desciptors found, so just compare by color
            FLANNtest = 0;
        }
        else
        {
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
             FLANNtest = 1 - avgdist;
        }
        
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
    else if(type == 1)
    {
        //normalize distances
        float z1 = (*(float *)input1->data - 5) / 3500;
        float z2 = (*(float *)input2->data - 5) / 3500;
        similarity = abs(z1 - z2);
    }
    else if(type >= 2 && type <= 4)
    {
        similarity = abs(*(float *)input1->data - *(float *)input2->data);
    }
    return similarity;
}
