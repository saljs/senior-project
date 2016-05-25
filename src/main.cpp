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
#include <ncurses.h>
#include <time.h>
#include "FANN/src/include/floatfann.h"
#include <mpi.h>
#include "p_database.h"

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
        return -1;
	}
	close(sockfd);
	return 0;
}

int main(int argc, char* argv[])
{
	//init MPI
	MPI_Init(NULL, NULL);
    // Find out rank, size
    int world_rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);
    int world_size;
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);
    if(world_rank != 0)
    {
		p_compare();
	}
	
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
    //time for incremental database saves
    time_t lastSave;
    time(&lastSave);
    
    //init ncurses window stuff
	WINDOW *display, *command;
    initscr();
    cbreak();
    noecho();
    nodelay(stdscr, TRUE);
    display = newwin(LINES, COLS/2, 0, 0);
    command = newwin(LINES, COLS/2, 0, COLS/2);
    idlok(display, TRUE);
    scrollok(display, TRUE);
    idlok(command, TRUE);
    scrollok(command, TRUE);
    
    while(true) //wait for robot to connect
    {
        wprintw(display, "Waiting for robot to connect...");
        wrefresh(display);
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
        wprintw(display, "Robot connected!\n");
        wrefresh(display);
        while(true) //main loop
        {	
			//send ready signal
			short int ready = 1;
			if(sendToRobot(&ready, sizeof(short int)) != 0)
			{
				error("ERROR reaching robot");
				continue;
			}
			wprintw(display, "Ready to communicate\n");
            wrefresh(display);
            //get inputs from robot
            input* newInputs[5];
            bool stopCond = false;
            for(int i = 0; i < 5; i++)
            {	
	    	    //read input dataSize
	    	    newInputs[i] = malloc(sizeof(input));
	    	    newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
                if(listenToRobot(newsockfd, &newInputs[i]->dataSize, sizeof(size_t)) != 0)
                {
                    error("ERROR robot is not responding");
                    stopCond = true;
                    break;
                }
				wprintw(display, "Recieved input %d size - %d\n", i, newInputs[i]->dataSize);
                wrefresh(display);
                newInputs[i]->data = malloc(newInputs[i]->dataSize);
	    	    //read data
	    	    newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
                if(listenToRobot(newsockfd, newInputs[i]->data, newInputs[i]->dataSize) != 0)
                {
                    error("ERROR robot is not responding");
                    stopCond = true;
                    break;
                }
                wprintw(display, "Recieved input %d data\n", i);
                wrefresh(display);
                if(i == 0)
                {
					//use parallell implemtation for image inputs
					p_linkInput(newInputs[i], i, database, world_size);
				}
				else
				{
					linkInput(newInputs[i], i, database);
				}
	    	}
            if(stopCond == true)
            {
                break;
            }
	    	//feed data into the input neurons
            int i = 0;
	    	for(; i < 4; i++)
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
            wprintw(display, "running data through neural net..\n");
            wrefresh(display);
            calc_out = fann_run(ann, inputNeurons);
            wprintw(display, "Output: %f %f\n", calc_out[0], calc_out[1]);
            wrefresh(display);
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
           
	    	if(sendToRobot(&netOutput, sizeof(instructions)) != 0)
	    	{
	    		error("ERROR comminicating with robot");
                break;
	    	}
	    	
	    	//listen for score
	    	float score;
	    	newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
	    	if(listenToRobot(newsockfd, &score, sizeof(float)) != 0)
            {
                error("ERROR robot is not responding");
                break;
            }
	    	wprintw(display, "Score = %f\n", score);
            wrefresh(display);
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
            
            //check if the database needs to be saved
            if(difftime(time(NULL), lastSave) > 60)
            {
				wprintw(command, "Saving the database...");
                wrefresh(command);
                int save = saveDatabase(DATABASE_F, database); 
                if(save != 0)
                {
                    error("error saving the database!\n");
                    wprintw(command, "Error code: %d\n", save);
                    wrefresh(command);
                }
                wprintw(command, "done!\n");
                wrefresh(command);
                //neural net
                wprintw(command, "Saving neural net...");
                wrefresh(command);
                fann_save(ann, FANN_F);
                wprintw(command, "done!\n");
                wrefresh(command);
                //tempurature
                wprintw(command, "Saving tempurature...");
                wrefresh(command);
                FILE* tempFile = fopen(TEMP_F, "w");
                fwrite(&temp, sizeof(double), 1, tempFile);
                fclose(tempFile);
                wprintw(command, "done!\n");
                wrefresh(command);
                time_t curtime;
                time(&curtime);
                struct tm *loctime = localtime(&curtime);
                lastSave = curtime;
                wprintw(command, "Database save completed at %s\n", asctime(loctime));
                wrefresh(command);
			}

        }
        wprintw(display, "Lost connection with robot\n");
        wrefresh(display);
        //save all the importatnt junk
        //database
        wprintw(display, "Saving the database...");
        wrefresh(display);
        int save = saveDatabase(DATABASE_F, database); 
        if(save != 0)
        {
            error("error saving the database!\n");
            wprintw(display, "Error code: %d\n", save);
            wrefresh(display);
        }
        wprintw(display, "done!\n");
        wrefresh(display);
        //neural net
        wprintw(display, "Saving neural net...");
        wrefresh(display);
        fann_save(ann, FANN_F);
        wprintw(display, "done!\n");
        wrefresh(display);
        //tempurature
        wprintw(display, "Saving tempurature...");
        wrefresh(display);
        FILE* tempFile = fopen(TEMP_F, "w");
        fwrite(&temp, sizeof(double), 1, tempFile);
        fclose(tempFile);
        wprintw(display, "done!\n");
        wrefresh(display);
    }
    delwin(display);
    delwin(command);
    endwin();
    MPI_Finalize();
    disassemble(database);
    close(sockfd);
    return 0;
}
