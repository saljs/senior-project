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

#define SERVER_PORT 8087
#define SEND_PORT 8085
#define ROBOT "robot"

void error(const char* message)
{
    fprintf(stderr, "%s - %s\n", message, strerror(errno));
    exit(0);
}

int sendToRobot(const void* buffer, size_t bufferLength)
{
    int sockfd, portno, n;
    struct sockaddr_in serv_addr;
    struct hostent *server;
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
    { 
        error("ERROR opening socket");
    }
    server = gethostbyname(ROBOT);
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
    n = read(sockfd,returnBuf,255);
    if (n < 0)
    {
        return -1;
    }
    close(sockfd);
    if(!strcmp(returnBuf, "OK"))
    {
		return -2;
	}
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
    }
    //init the ANN
    struct fann *ann = fann_create_standard(LAYERS, (NUMINPUTS-1) * DBINPUTS, HIDDEN, 2);
    fann_set_activation_function_hidden(ann, FANN_SIGMOID);
    fann_set_activation_function_output(ann, FANN_SIGMOID);
    fann_type *calc_out;
    fann_type inputNeurons[(NUMINPUTS-1)*DBINOUTS];
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
    serv_addr.sin_port = htons(SERVER_PORT);
    if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
    {
        error("ERROR on binding");
    }

	
    while(true)
    {
		//send ready signal
		short int ready = 1;
		if(!sendToRobot(&ready, sizeof(short int)))
		{
			error("ERROR comminicating with robot");
		}
        //get inputs
        input* text = getInput(0, database);
        printf("\n");
        input* image = getInput(1, database);
        printf("\n");
        if(text == NULL && image == NULL)
        {
            //exit the program
            break;
        }
        else if(text == NULL && image != NULL)
        {
            //outputting text based on image
            memory* composite = NULL;
            compileMem(image, database, &composite, 1);
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
            memory* composite = NULL;
            compileMem(text, database, &composite, 1);
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
                namedWindow( "Image", WINDOW_AUTOSIZE );
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
    }
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
    }
    disassemble(database);
    return 0;
}
