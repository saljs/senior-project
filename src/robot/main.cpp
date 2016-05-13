typedef struct instructions   //instructrions for robot from neural net
{
    short int direction;
    short int  steering;
} instructions;


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
    instructions serverInput;

    //init sender socket
    int senderSock, j;
    struct sockaddr_in Send_addr;
    struct hostent *server;
    senderSock = socket(AF_INET, SOCK_STREAM, 0);
    if (senderSock < 0) 
    {
        error("ERROR opening socket");
    }
    server = gethostbyname(SEND_PORT);
    if (server == NULL) {
        fprintf(stderr,"ERROR, no such host\n");
        exit(0);
    }
    bzero((char *) &Send_addr, sizeof(Send_addr));
    Send_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, (char *)&Send_addr.sin_addr.s_addr, server->h_length);
    Send_addr.sin_port = htons(portno);
    if (connect(senderSock,(struct sockaddr *) &Send_addr,sizeof(Send_addr)) < 0) 
    {
        error("ERROR connecting");
    }

    while(true) //main loop
    {

        listen(sockfd,5);   //listen for ready signal
        //handle server data
        clilen = sizeof(cli_addr);
        newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
        if (newsockfd < 0)
        {
            error("ERROR on accept");
        }
        short int ready = 0;
        n = read(newsockfd,ready,sizeof(short int));
        if (n < 0)
        {
            error("ERROR reading from socket";
        }
        char returnHead[] = "OK";
        n = write(newsockfd,returnHead,strlen(returnHead));
        if (n < 0) 
        {
            error("ERROR writing to socket");
        }
        
        //check if the server is indeed ready
        if(ready != 1)
        {
            continue;
        }        


        listen(sockfd,5);   //listen for instructions
        //handle server data
        clilen = sizeof(cli_addr);
        newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
        if (newsockfd < 0)
        {
            error("ERROR on accept");
        }
        bzero(serverInput,sizeof(instructions));
        n = read(newsockfd,serverInput,sizeof(instuctions));
        if (n < 0)
        {
            error("ERROR reading from socket";
        }
        returnHead[] = "OK";
        n = write(newsockfd,returnHead,strlen(returnHead));
        if (n < 0) 
        {
            error("ERROR writing to socket");
        }

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
        
        if(serverInput.direction == 1)
        {
            //forward
            digitalWrite(MOTORS, 0); //for sanity
            digitalWrite(MOTOR_L, 0);
            digitalWrite(MOTOR_R, 0);
            digitalWrite(MOTORS, 1);
            delay(DRIVE_DUR);
            digitalWrite(MOTORS, 0);
        }
        if(serverInput.direction == 2)
        {
            //backwards
            digitalWrite(MOTORS, 0); //for sanity
            digitalWrite(MOTOR_L, 1);
            digitalWrite(MOTOR_R, 1);
            digitalWrite(MOTORS, 1);
            delay(DRIVE_DUR);
            digitalWrite(MOTORS, 0);
        }
        
	}
}

