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

typedef struct instructions   //instructrions for robot from neural net
{
    short int direction;
    short int  steering;
} instructions;

int sendToServer(int socket, const void* buffer, size_t bufferLength)
{
	int n = write(socket, buffer, bufferLength);
    if (n < 0) 
    {
        return 1;
    }
    char returnBuf[255];
    n = read(socket,returnBuf,255);
    if (n < 0)
    {
        return -1;
    }
    if(!strcmp(returnBuf, "OK"))
    {
		return -2;
	}
	return 0;
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

void error(const char* message)
{
    fputs(message, stderr);
    exit(0);
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
    return 0;
}
        
int main(int argc, char** argv)
{
    //init hardware
    
    //init listener socket socket
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

    //init sender socket
    int senderSock;
    struct sockaddr_in Send_addr;
    struct hostent *server;
    senderSock = socket(AF_INET, SOCK_STREAM, 0);
    if (senderSock < 0) 
    {
        error("ERROR opening socket");
    }
    server = gethostbyname(SERVER);
    if (server == NULL) {
        fprintf(stderr,"ERROR, no such host\n");
        exit(0);
    }
    bzero((char *) &Send_addr, sizeof(Send_addr));
    Send_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, (char *)&Send_addr.sin_addr.s_addr, server->h_length);
    Send_addr.sin_port = htons(SEND_PORT);
    if (connect(senderSock,(struct sockaddr *) &Send_addr,sizeof(Send_addr)) < 0) 
    {
        error("ERROR connecting");
    }

    while(true) //main loop
    {

        listen(sockfd,5);   //listen for ready signal
        digitalWrite(STATUS_LED, 1);
        //handle server data
        clilen = sizeof(cli_addr);
        newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
        if (newsockfd < 0)
        {
            error("ERROR on accept");
        }
        short int ready = 0;
        n = read(newsockfd,&ready,sizeof(short int));
        if (n < 0)
        {
            error("ERROR reading from socket");
        }
        char returnHead[] = "OK";
        n = write(newsockfd,returnHead,strlen(returnHead));
        if (n < 0) 
        {
            error("ERROR writing to socket");
        }
        digitalWrite(STATUS_LED, 0);
        //check if the server is indeed ready
        if(ready != 1)
        {
            //server is telling us to exit cleanly.
            break;
        }        
        
        //send inputs to server
        memory* dummy = newMemory(NULL);
        input* newInput;
        for(int i = 0; i < 5; i++)
        {
			newInput = getInput(i, dummy);
            digitalWrite(STATUS_LED, 1);
			if(!sendToServer(senderSock, newInput, sizeof(input)))
			{
				error("ERROR sending to server");
			}
			if(!sendToServer(senderSock, newInput->data, newInput->dataSize))
			{
				error("ERROR sending to server");
			}
            digitalWrite(STATUS_LED, 0);
		}
		free(newInput->data);
        free(newInput);
        disassemble(dummy);

        listen(sockfd,5);   //listen for instructions
        digitalWrite(STATUS_LED, 1);
        //handle server data
        clilen = sizeof(cli_addr);
        newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
        if (newsockfd < 0)
        {
            error("ERROR on accept");
        }
        instructions serverInput;
        bzero(&serverInput,sizeof(instructions));
        n = read(newsockfd,&serverInput,sizeof(instructions));
        if (n < 0)
        {
            error("ERROR reading from socket");
        }
        n = write(newsockfd,returnHead,strlen(returnHead));
        if (n < 0) 
        {
            error("ERROR writing to socket");
        }
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
        score += ((float)analogRead(SOLAR_T) / 1024.0) / 2;
        if(!sendToServer(senderSock, &score, sizeof(float)))
        {
            error("ERROR sending to server");
        }
	}
    stopCapture();
    close(senderSock);
    close(sockfd);
    return 0;
}

