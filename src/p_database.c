#include "p_database.h"
#include "database.h"
#include "inputs.h"
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <errno.h>
#include <stdbool.h>
#include <math.h>
#include <mpi.h>

void p_compare()
{
    int flag = 0;
    while(true)
    {
        while(!flag)
        {
            MPI_Iprobe(0, 0, MPI_COMM_WORLD, &flag, MPI_STATUS_IGNORE);
        }
        int number_amount;
        input* input1 = (input*)malloc(sizeof(input));
        input* input2 = (input*)malloc(sizeof(input));
        MPI_Datatype inputBuffer;
        MPI_Type_contiguous((int)sizeof(input), MPI_BYTE, &inputBuffer);
        MPI_Type_commit(&inputBuffer);

        MPI_Recv(input1, 1, inputBuffer, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

        input1->data = malloc(input1->dataSize);
        MPI_Recv(input1->data, (int)input1->dataSize, MPI_BYTE, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

        MPI_Recv(input2, 1, inputBuffer, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

        input2->data = malloc(input2->dataSize);
        MPI_Recv(input2->data, (int)input2->dataSize, MPI_BYTE, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

        int type;
        MPI_Recv(&type, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

        float similarity = compareInputs(input1, input2, type);
        MPI_Send(&similarity, 1, MPI_FLOAT, 0, 0, MPI_COMM_WORLD);
        free(input1->data);
        free(input1);
        free(input2->data);
        free(input2);
    }
}



void p_linkInput(input* pattern, int type, memory* database, int world_size)
{
    MPI_Datatype inputBuffer;
    MPI_Type_contiguous((int)sizeof(input), MPI_BYTE, &inputBuffer);
    MPI_Type_commit(&inputBuffer);
    float cost = 0, lastCost = 0, simConf = 0;
    memory* next = database;
    int index = 1;
    long int steps = 0, mostSim = -1, tmpuuid;
    input* inputList[world_size];
    long int uuidList[world_size];
    //zero input list
    for(int i = 0; i < world_size; i++)
    {
        inputList[i] = NULL;
    }
    do
    {
        float matchprob = 0;
        //select a batch of world_size inputs
        input* currInput = next->inputs[type];

        input* tmpSim;
        while(currInput != NULL)
        {
            inputList[index] = currInput;
            uuidList[index] = next->uuid;
            if(index == world_size)
            {
                float similarity[world_size];
                MPI_Request handles[world_size];
                //now that we have gathered enough values, send them to nodes
                for(int i = 1; i < world_size; i++)
                {
                    MPI_Send(pattern, 1, inputBuffer, i, 0, MPI_COMM_WORLD);
                    MPI_Send(pattern->data, pattern->dataSize, MPI_BYTE, i, 0, MPI_COMM_WORLD);
                    MPI_Send(inputList[i], 1, inputBuffer, i, 0, MPI_COMM_WORLD);
                    MPI_Send(inputList[i]->data, inputList[i]->dataSize, MPI_BYTE, i, 0, MPI_COMM_WORLD);
                    MPI_Send(&type, 1, MPI_INT, i, 0, MPI_COMM_WORLD);
                    MPI_Irecv(&similarity[i], 1, MPI_FLOAT, i, 0, MPI_COMM_WORLD, &handles[i]);
                }
                for(int i = 1; i < world_size; i++)
                {
                    //now wait for all those things to return
                    MPI_Wait(&handles[i], MPI_STATUS_IGNORE);
                    if(similarity[i] > matchprob) //if the input is the most simalimar of all inputs in the memory so far, save it
                    {
                        matchprob = similarity[i];
                        tmpSim = inputList[i];
                        tmpuuid = uuidList[i];
                    }
                }
                index = 0;
                //zero input list
                for(int i = 0; i < world_size; i++)
                {
                    inputList[i] = NULL;
                }
            }
            index++;
            input* tmp = currInput->next;
            currInput = tmp;
        }
        if(matchprob > simConf) //if the current similar is the most similar so far, save it
        {
            mostSim = tmpuuid;
            simConf = matchprob;
        }
        lastCost = cost;
        steps++;

        cost = steps / (matchprob + 1); //calculate algorithm cost
        if(matchprob > BRANCH_LIMIT && index == 1) //if the current memory is more similar than the BRANCH_LIMIT, go to the linked memory instead of iterating linearly
        {
            memory* loop = next;
            while(loop->uuid > tmpSim->link)
            {
                memory* tmp = loop->next;
                loop = tmp;
                if(loop == NULL)
                {
                    break;
                }
            }
            next = loop;
        }
        else //iteralte over memory list linearly
        {
            memory* tmp = next->next;
            next = tmp;
        }
    } while(cost * STOP_LIMIT > lastCost && next != NULL); //check stop conditions
    //check to see if there are any leftovers that haven't been processed yet
    if(inputList[1] != NULL)
    {
        int remainderSize;
        for(int i = 1; i < world_size; i++)
        {
            if(inputList[i] == NULL)
            {
                remainderSize = i;
                break;
            }
        }
        float similarity[remainderSize];
        MPI_Request handles[remainderSize];
        //now that we have gathered enough values, send them to nodes
        for(int i = 1; i < remainderSize; i++)
        {
            MPI_Send(pattern, 1, inputBuffer, i, 0, MPI_COMM_WORLD);
            MPI_Send(pattern->data, pattern->dataSize, MPI_BYTE, i, 0, MPI_COMM_WORLD);
            MPI_Send(inputList[i], 1, inputBuffer, i, 0, MPI_COMM_WORLD);
            MPI_Send(inputList[i]->data, inputList[i]->dataSize, MPI_BYTE, i, 0, MPI_COMM_WORLD);
            MPI_Send(&type, 1, MPI_INT, i, 0, MPI_COMM_WORLD);
            MPI_Irecv(&similarity[i], 1, MPI_FLOAT, i, 0, MPI_COMM_WORLD, &handles[i]);
        }
        for(int i = 1; i < remainderSize; i++)
        {
            //now wait for all those things to return
            MPI_Wait(&handles[i], MPI_STATUS_IGNORE);
            if(similarity[i] > simConf) //if the input is the most simalimar of all inputs in the memory so far, save it
            {
                mostSim = uuidList[i];
                simConf = similarity[i];
            }
        }
    }
    pattern->link = mostSim; //link input to the most similar one found
    pattern->confidence = simConf;
    return;
}


/*
memory* p_AddtoMem(input* newInput, int index, memory* database, bool* newTrigger)
{
    int count = 0;
    float mean = 0.0, sum = 0.0;
    input* next = database->inputs[index];
    while(next != NULL)
    {
        sum += compareInputs(newInput, next, index);
        count++;
        input* tmp = next->next;
        next = tmp;
    }
    mean = sum / count; //calculate mean similarity to the inputs currently in the memory
    memory* updated;
    if(mean > SPLIT_LIMIT || isnan(mean)) //append the input to current memory
    {
        if(newTrigger != NULL)
        {
            *newTrigger = false;
        }
        addInput(database, newInput, index);
        updated = database;
    }
    else //split the input into a new memory
    {
       if(newTrigger != NULL)
       {
           *newTrigger = true;
       }
       memory* newStart = newMemory(database);
       if(newStart == NULL)
       {
           return NULL;
       }
       addInput(newStart, newInput, index);
       updated = newStart;
    }
    return updated;
}
*/
