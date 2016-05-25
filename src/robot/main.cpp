#include "database.h"
#include "inputs.h"
#include "vars.h"
#include "hardware.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <wiringPi.h>
#include <mcp3004.h>
#include <errno.h>

typedef struct instructions   //instructrions for robot from neural net
{
    short int direction;
    short int  steering;
} instructions;

void error(const char* message)
{
    fprintf(stderr, "%s - %s\n", message, strerror(errno));
    exit(0);
}

int sendToServer(const void* buffer, size_t bufferLength)
{
    int sockfd, portno, n;
    struct sockaddr_in serv_addr;
    struct hostent *server;
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
    { 
        error("ERROR opening socket");
    }
    server = gethostbyname(SERVER);
    if(server == NULL) 
    {
        error("ERROR, no such host");
    }
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);
    serv_addr.sin_port = htons(SEND_PORT);
    if (connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0) 
    {
        error("ERROR connecting");
    }
    n = write(sockfd, buffer, bufferLength);
    if (n < 0) 
    {
        return 1;
    }
    char returnBuf[255];
    bzero(returnBuf, 255);
    n = read(sockfd,returnBuf,255);
    if (n < 0)
    {
        return -1;
    }
    close(sockfd);
    if(strncmp(returnBuf, "OK", 2) != 0)
    {
		return -2;
	}
	return 0;
}
void listenToServer(int sockfd, void* buffer, size_t bufferLength)
{
	if (sockfd < 0)
	{
		error("ERROR on accept");
	}
	void* ptr = buffer;
    int n;
    int totalRead  = 0;
    while(totalRead < bufferLength)
    {
    	n = read(sockfd, ptr, bufferLength);
    	if (n < 0)
    	{
    		error("ERROR reading from socket");
            return -1;
    	}
        totalRead += n;
        ptr += n;
    }
	char returnHead[] = "OK";
	n = write(sockfd, returnHead, strlen(returnHead));
	if (n < 0) 
	{
		error("ERROR writing to socket");
	}
	close(sockfd);
	return;
}

float getDistance(int trig, int echo)
{
    //Send trig pulse
    digitalWrite(trig, HIGH);
    delayMicroseconds(10);
    digitalWrite(trig, LOW);

    //Wait for echo start
    while(digitalRead(echo) == LOW);

    //Wait for echo end
    long startTime = micros();
    while(digitalRead(echo) == HIGH);
    long travelTime = micros() - startTime;

    //Get distance in cm
    float distance = travelTime * 0.01715;

    return distance;
}

int setupHardware()
{
    if (wiringPiSetup () == -1)
    {
        return 1;
    }
    mcp3004Setup(200, 0);
    pinMode(STATUS_LED, OUTPUT);
    pinMode(MOTORS, OUTPUT);
    pinMode(MOTOR_L, OUTPUT);
    pinMode(MOTOR_R, OUTPUT);
    pinMode(TRIG, OUTPUT);
    pinMode(ECHO, INPUT);
    pinMode(TRIG1, OUTPUT);
    pinMode(ECHO1, INPUT);
    startCapture();
    delay(1000); //give the camera time to start up
    return 0;
}
        
int main(int argc, char** argv)
{
    //init hardware
    if(setupHardware() == 1)
    {
        error("error setting up hardware");
    }
    //init listener socket 
    int sockfd, newsockfd;
    socklen_t clilen;
    struct sockaddr_in serv_addr, cli_addr;
    int n;
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
    {
        error("ERROR opening socket");
    }
    bzero((char *) &serv_addr, sizeof(serv_addr));

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(LISTEN_PORT);
    if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
    {
        error("ERROR on binding");
    }
    listen(sockfd,5);
    clilen = sizeof(cli_addr);

    float lastLval = 0;

    //send on signal to the server
    short int connected = 1;
    if(sendToServer(&connected, sizeof(short int)) != 0)
    {
        error("ERROR server not responding");
    }

    while(true) //main loop
    {   
		//listen for ready signal
		newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
		digitalWrite(STATUS_LED, 1);
		short int ready = 0;
		listenToServer(newsockfd, &ready, sizeof(short int));
		//check if the server is indeed ready
		if(ready != 1)
		{
		    error("ERROR something went terribly wrong");	
		} 
		digitalWrite(STATUS_LED, 0);
        //send inputs to server
        memory* dummy = newMemory(NULL);
        input* newInput;
        for(int i = 0; i < 5; i++)
        {
			digitalWrite(STATUS_LED, 1);
			newInput = getInput(i, dummy);
			unsigned long long int longSize = newInput->dataSize;
			if(sendToServer(&longSize, sizeof(unsigned long long int)) != 0) //size_t is defined as this on x86_64
			{
				error("ERROR sending to server");
			}
		    if(sendToServer(newInput->data, newInput->dataSize) != 0)
		    {
		    	error("ERROR sending to server");
		    }
            digitalWrite(STATUS_LED, 0);
		}
		free(newInput->data);
        free(newInput);
        disassemble(dummy);

        //listen for instructions
        instructions serverInput;
        bzero(&serverInput,sizeof(instructions));
		newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
        digitalWrite(STATUS_LED, 1);
        listenToServer(newsockfd, &serverInput, sizeof(instructions));
        digitalWrite(STATUS_LED, 0);

        //apply server instructions
        if(serverInput.steering == 1)
        {
            //turn left
            digitalWrite(MOTORS, 0); //for sanity
            digitalWrite(MOTOR_L, 1);
            digitalWrite(MOTOR_R, 0);
            digitalWrite(MOTORS, 1);
            delay(TURN_DUR);
            digitalWrite(MOTORS, 0);
        }
        else if(serverInput.steering == 2)
        {
            //turn right
            digitalWrite(MOTORS, 0); //for sanity
            digitalWrite(MOTOR_L, 0);
            digitalWrite(MOTOR_R, 1);
            digitalWrite(MOTORS, 1);
            delay(TURN_DUR);
            digitalWrite(MOTORS, 0);
        }
        
        float score = 0.5;
        if(getDistance(TRIG, ECHO) < 7)
        {
            //decrease score because you crashed into stuff
            score -= 0.25;
        }
        else if(serverInput.direction == 1)
        {
            //forward
            digitalWrite(MOTORS, 0); //for sanity
            digitalWrite(MOTOR_L, 0);
            digitalWrite(MOTOR_R, 0);
            digitalWrite(MOTORS, 1);
            delay(DRIVE_DUR);
            digitalWrite(MOTORS, 0);
        }
        if(getDistance(TRIG1, ECHO1) < 7)
        {
            //decrease score because you backed into stuff
            score -= 0.25;
        }
        else if(serverInput.direction == 2)
        {
            //backwards
            digitalWrite(MOTORS, 0); //for sanity
            digitalWrite(MOTOR_L, 1);
            digitalWrite(MOTOR_R, 1);
            digitalWrite(MOTORS, 1);
            delay(DRIVE_DUR);
            digitalWrite(MOTORS, 0);
        }
        
        //calculate new score
        //subtract points for hitting stuff
        //add points for direct light 

        float Lval = (float)analogRead(SOLAR_T) / 1024.0;
        score += Lval - lastLval;
        if(score < 0)
        {
            score = 0;
        }
        else if(score > 1)
        {
            score = 1;
        }
        lastLval = Lval;

        if(sendToServer(&score, sizeof(float)) != 0)
        {
            error("ERROR sending to server");
        }
	}
    stopCapture();
    close(sockfd);
    return 0;
}

