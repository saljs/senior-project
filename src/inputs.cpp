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

using namespace std;
using namespace cv;

input* getInput(int type, memory* database)
{
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
    else if(type >= 1 && type <= 5) //distance sensor, light sensors and score
    {
        similarity = abs(*(float *)input1->data - *(float *)input2->data);
    }
    return similarity;
}
