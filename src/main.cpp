#include "database.h"
#include "inputs.h"
#include "vars.h"
#include "FANNvars.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/socket.h>
#include <netdb.h>
#include <errno.h>
#include "FANN/src/include/floatfann.h"

#include <iostream>
#include "opencv2/core/core.hpp"
#include "opencv2/highgui/highgui.hpp"

using namespace std;
using namespace cv;

#define LISTEN_PORT 8087
#define SEND_PORT 8085
#define ROBOT "robot"
#define DATABASE_F "database.dlsd"
#define FANN_F "neural.net"
#define TEMP_F "temp.file"

void error(const char* message)
{
    fprintf(stderr, "%s - %s\n", message, strerror(errno));
}

typedef struct instructions   //instructrions for robot from neural net
{
    short int direction;
    short int  steering;
} instructions;

int sendToRobot(const void* buffer, size_t bufferLength)
{
    int sockfd, portno, n;
    struct sockaddr_in serv_addr;
    struct hostent *server;
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
    { 
        error("ERROR opening socket");
        return -1;
    }
    server = gethostbyname(ROBOT);
    if(server == NULL) 
    {
        error("ERROR, no such host");
        return -1;
    }
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);
    serv_addr.sin_port = htons(SEND_PORT);
    if (connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0) 
    {
        error("ERROR connecting");
        return -1;
    }
	n = write(sockfd, buffer, bufferLength);
    if (n < 0) 
    {
        return -1;
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
		return -1;
	}
	return 0;
}

int listenToRobot(int sockfd, void* buffer, size_t bufferLength)
{
	if (sockfd < 0)
	{
		error("ERROR on accept");
        return -1;
	}
	int n = read(sockfd, buffer, bufferLength);
	if (n < 0)
	{
		error("ERROR reading from socket");
        return -1;
	}
	char returnHead[] = "OK";
	n = write(sockfd, returnHead, strlen(returnHead));
	if (n < 0) 
	{
		error("ERROR writing to socket");
        return -1;
	}
	close(sockfd);
	return 0;
}

void visualizer(memory* database) //prints ascii representation of database
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
    //init the database
    memory* database;
    if(access(DATABASE_F, F_OK) == -1) 
    {
        database = newMemory(NULL);
    }
    else //load database from file
    {
        database = loadDatabase(DATABASE_F);
    }
    if(database == NULL)
    {
        error("problem with the database\n");
        return 1;
    }
    //init the ANN
    struct fann *ann;
    if(access(FANN_F, F_OK) == -1)
    {
        ann = fann_create_standard(LAYERS, 4+((NUMINPUTS-1) * DBINPUTS), HIDDEN, 2);
        fann_set_activation_function_hidden(ann, FANN_SIGMOID);
        fann_set_activation_function_output(ann, FANN_SIGMOID);
    }
    else
    {
        ann = fann_create_from_file(FANN_F);
    }
    
    fann_type *calc_out;
    fann_type inputNeurons[4+((NUMINPUTS-1)*DBINPUTS)];
    //read tempurature from file if it exists
    double temp = 1.0;
    if(access(TEMP_F, F_OK) != -1)
    {
        FILE* tempFile = fopen(TEMP_F, "r");
        fread(&temp, sizeof(double), 1, tempFile);
        fclose(tempFile);
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
        return -1;
    }
    bzero((char *) &serv_addr, sizeof(serv_addr));

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(LISTEN_PORT);
    if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
    {
        error("ERROR on binding");
        return -1;
    }
    listen(sockfd,5);
    clilen = sizeof(cli_addr);
    
    //init the rng
    srandom(time(NULL));    
    //define some variables for the annealing algorithm
    float lastCost = INFINITY;
    int lastWeight;
    fann_type lastVal;
    while(true) //wait for robot to connect
    {
        printf("Waiting for robot to connect...");
        newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
        short int connected;
        if(listenToRobot(newsockfd, &connected, sizeof(short int)) != 0)
        {
            error("ERROR comminicating with robot");
            continue;
        }
        if(connected != 1)
        {
            error("ERROR robot sent unrecognized connected signal");
            continue;
        }
        printf("Robot connected!\n");
        while(true) //main loop
        {	
            //get inputs from robot
            input* newInputs[4];
            bool stopCond = false;
            for(int i = 0; i < 5; i++)
            {
	    		newInputs[i] = malloc(sizeof(input));
	    		//ready to recieve data
	    		short int ready = 1;
	    		if(sendToRobot(&ready, sizeof(short int)) != 0)
	    		{
	    			error("ERROR comminicating with robot");
                    stopCond = true;
                    break;
	    	    }
	    	    //read input
	    	    newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
                if(listenToRobot(newsockfd, newInputs[i], sizeof(input)) != 0)
                {
                    error("ERROR robot is not responding");
                    stopCond = true;
                    break;
                }
              
                void* tmp = malloc(newInputs[i]->dataSize);
                //ready to recieve data
	    		if(sendToRobot(&ready, sizeof(short int)) != 0)
	    		{
	    			error("ERROR comminicating with robot");
                    stopCond = true;
                    break;
	    	    }
	    	    //read data
	    	    newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
                if(listenToRobot(newsockfd, tmp, newInputs[i]->dataSize) != 0)
                {
                    error("ERROR robot is not responding");
                    stopCond = true;
                    break;
                }
                newInputs[i]->data = tmp;
                linkInput(newInputs[i], i, database);
	    	}
            if(stopCond == true)
            {
                break;
            }
	    	//feed data into the input neurons
            int i = 0;
	    	for(; i < 5; i++)
            {
                inputNeurons[i] = *(float *)newInputs[i+1]->data;
            }
            //generate a composite memory
            memory* composite = NULL;
            compileMem(newInputs[0], database, &composite, DBINPUTS);
            memory* next = composite;
            for(int j = 0; j < DBINPUTS; j++)
            {
                for(int k = 1; k < 6; k++)
                {
                    if(next == NULL)
                    {
                        inputNeurons[i] = 0;
                    }
                    else
                    {
                        if(next->inputs[k] == NULL)
                        {
                            inputNeurons[i] = 0;
                        }
                        else
                        {
                            inputNeurons[i] = *(float *)next->inputs[k]->data;
                        }
                        next = next->next;
                    }
                    i++;
                }
            }
            disassemble(composite);
            calc_out = fann_run(ann, inputNeurons);
            //translate output to instructions for the robot
            instructions netOutput;
            if(calc_out[0] < 1/3)
            {
                netOutput.direction = 2;
            }
            else if(calc_out[0] >= 1/3 && calc_out[0] <= 2/3)
            {
                netOutput.direction = 0;
            }
            else if(calc_out[0] > 2/3)
            {
                netOutput.direction = 1;
            }

            if(calc_out[1] < 1/3)
            {
                netOutput.steering = 2;
            }
            else if(calc_out[1] >= 1/3 && calc_out[1] <= 2/3)
            {
                netOutput.steering = 0;
            }
            else if(calc_out[1] > 2/3)
            {
                netOutput.steering = 1;
            }
            //listen for ready signal
	    	newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
	    	short int ready = 0;
	    	if(listenToRobot(newsockfd, &ready, sizeof(short int)) != 0)
	    	{
				error("ERROR robot is not responding");
                break;
			}
	    	//check if the server is indeed ready
	    	if(ready != 1)
	    	{
	    		//something went terribly wrong.
	    		error("ERROR something went terribly wrong");
	    		exit(1);
	    	}
	    	if(sendToRobot(&netOutput, sizeof(instructions)) != 0)
	    	{
	    		error("ERROR comminicating with robot");
                break;
	    	}
	    	
	    	//tell the robot we are ready to recive its score
	    	ready = 1;
	    	if(sendToRobot(&ready, sizeof(short int)) != 0)
	    	{
	    		error("ERROR comminicating with robot");
                break;
	    	}
	    	float score;
	    	newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
	    	if(listenToRobot(newsockfd, &score, sizeof(float)) != 0)
            {
                error("ERROR robot is not responding");
                break;
            }
	    	
	    	//add inputs to database, starting with score
	    	input* scoreIn = malloc(sizeof(input));
	    	scoreIn->data = malloc(sizeof(float));
            *(float *)scoreIn->data = score;
            scoreIn->dataSize = sizeof(float);
            linkInput(scoreIn, NUMINPUTS-1, database);
            database = AddtoMem(scoreIn, NUMINPUTS-1, database, NULL);
            for(int j = 0; j < NUMINPUTS - 1; j++)
            {
                database = AddtoMem(newInputs[j], j, database, NULL);
            }

            //anneal the net
            float cost = 1 - score;
            float a = M_E * ((lastCost - cost) / temp);
            lastCost = cost;
            int weight;
            fann_type val;
            if(a <= (float)random() / (double)RAND_MAX)
            {
                //go back to previous solution
                weight = lastWeight;
                val = lastVal;
            }
            else
            {

                //try new solution
                weight = random() % ann->total_connections;
                val = ann->weights[weight];
                if(random() > RAND_MAX/2)
                {
                    val += MOD_VAL;
                }
                else
                {
                    val -= MOD_VAL;
                }
            }
            //save current value
            lastWeight = weight;
            lastVal = val;
            //change value to new/old one
            ann->weights[weight] = val;
            //decrease temperature
            temp = temp * ALPHA;
        }
        printf("Lost connection with robot\n");
        //save all the importatnt junk
        //database
        printf("Saving the database...");
        int save = saveDatabase(DATABASE_F, database); 
        if(save != 0)
        {
            error("error saving the database!\n");
            printf("Error code: %d\n", save);
        }
        printf("done!\n");
        //neural net
        printf("Saving neural net...");
        fann_save(ann, FANN_F);
        printf("done!\n");
        //tempurature
        printf("Saving tempurature...");
        FILE* tempFile = fopen(TEMP_F, "w");
        fwrite(&temp, sizeof(double), 1, tempFile);
        fclose(tempFile);
        printf("done!\n");
    }
    /*
    if(argc < 2) //overwrite preexiting database file
    {
        int save = saveDatabase("database", database); 
        if(save != 0)
        {
            error("error saving the database!\n");
            printf("Error code: %d\n", save);
        }         
    }
    else //create new database file
    {
        int save = saveDatabase(argv[1], database);
        if(save != 0)
        {
            error("error saving the database!\n");
            printf("Error code: %d\n", save);
        }         
    }*/
    disassemble(database);
    return 0;
}
