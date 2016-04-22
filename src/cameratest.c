#include "opencv2/opencv.hpp"
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>

using namespace cv;
int exitBool = 0;
Mat frame;
void* showimg(void* arg)
{
    VideoCapture cap;
    // open the default camera, use something different from 0 otherwise;
    // Check VideoCapture documentation.
    if(!cap.open(0))
    {
        return 0;
    }
    for(;;)
    {
        //loop to get videa frames as they come in
        cap >> frame;
        if(frame.empty() || exitBool == 1) break; // end of video stream
    }
    return NULL;
}

int main(int argc, char** argv)
{
    pthread_t thread;
    pthread_create(&thread, NULL, showimg, NULL);
    sleep(5);
    for(;;)
    {
        if(waitKey(0) == 27)
        {
            exitBool = 1;
            break;
        }
        imshow("camera", frame);
    }
    // the camera will be closed automatically upon exit
    // cap.close();
    pthread_join(thread, NULL);
    return 0;
}
