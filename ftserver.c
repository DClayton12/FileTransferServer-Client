/* 
 * Darnel Clayton
 * Title: ftserver.c
 * CS 372; Intro to Networks
 * Description: This program establishes a TCP connection with a ftclient.java. 
 * This server application reads in a message buffer through a socket as commands.
 * Specified error handling is included. 
 * The server will fork and continue to listen on the specified port.
 * Referened: 
 * http://www.linuxhowtos.org/C_C++/socket.htm
 * http://beej.us/guide/bgnet/output/print/bgnet_USLetter_2.pdf
 * CS 344; Operating Systems lecture and assignment 4.
 * http://stackoverflow.com/questions/2014033/send-and-receive-a-file-in-socket-programming-in-linux-with-c-c-gcc-g
 */

#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <netdb.h>
#include <netinet/in.h>
#include <signal.h>

/*
 * Description: Validates user input to be valid. Either '-g' or '-l'
 * If user selection is '-g' filename is stored for later data transfer.
 * Client is prompted.
 * Acknowledgement sent to client.
 */
void getRequest(int sock, char* cmd, char* fname, char *response);

/*
 * Description: Transfers requested data over new connection.
 * Connection is closed when logic terminates. 
 * Returns to main and original communication connection on original port is also closed.
 * Control returns to infinite while loop in main() so they server may continue to listen.
 */
void dataTransfer(int sock, char* comm, char* fname, int portnum, char* hostName);


int main(int argc, char *argv[]){

	int sockfd, newsockfd, datasockfd, portno, pid; 
	socklen_t clilen;
	struct sockaddr_in serv_addr2; 
    	struct sockaddr_in serv_addr, cli_addr; // Socket dedicated for TCP communication and for Data transfer to client.
    	struct hostent *server;
	char userCMD[10] = ""; // Eval to user input to be valid.
        char dportno[10] = "";
        char command[10] ="";
        char fileName[50]= "";
        char hostname[1024]; // Referenced: http://stackoverflow.com/questions/504810/how-do-i-find-the-current-machines-full-hostname-in-c-hostname-and-domain-info
	 if (argc < 2) { // USAGE: ftserver <PORT NUMBER>
		fprintf(stderr,"Error: PORT NUMBER NOT SPECIFIED. EXITING.\n");
        	exit(1);
    	 }

	while(1){
		sockfd = socket(AF_INET, SOCK_STREAM, 0);	
		if (sockfd < 0) {
			fprintf(stderr, "ERROR: Opening socket, EXITING.\n");
			exit(1);
		}

		bzero((char *) &serv_addr, sizeof(serv_addr));
		portno = atoi(argv[1]); // Storing port number from command line for later use.

		int yes=1;
		if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) == -1) { // Segment collaborated with Jon Derderian.
			fprintf(stderr, "setsockopt\n");
			exit(1);
		}

		serv_addr.sin_family = AF_INET; // Setting up server info. (TCP.)
		serv_addr.sin_addr.s_addr = INADDR_ANY;
		serv_addr.sin_port = htons(portno);

		if (bind(sockfd, (struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0) { // Referenced: CS 344 Lecture.
			fprintf(stderr, "ERROR: on binding.\n");
			exit(1);
		}
		
		listen(sockfd,10); // Server waiting for client on specified port.
		clilen = sizeof(cli_addr); // Referenced CS 344 lecture.
		
		newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen); // Establish conncection w/ client. Referenced CS 344 lecture.

		if (newsockfd < 0) {
			fprintf(stderr, "ERROR: on accept. Connection closed. EXITING.\n");
			close(newsockfd); // Close upon error and exit.
			close(sockfd);
			exit(1);
		}

		printf("Client is connected and ready to com,unicate. Server is running on port: %d\n", portno);
		
		int x=0;
		x=read(newsockfd,dportno,sizeof(dportno)); // Read in data port number from client. Number of bytes read in store in x for error handling.
		if (x < 0){
			fprintf(stderr, "ERROR: reading from socket.\n");
			close(newsockfd);
			exit(1);
		}
	
		x = write(newsockfd,"Recieved Confirmed.",20); // Write confirmation to client as ACK. 
		if (x < 0){
			fprintf(stderr, "ERROR: writing to socket: %d\n", x);
			close(newsockfd);
			exit(1);
		}
		printf("Data transfer connection ready on port: %s\n", dportno);

		getRequest(newsockfd, command, fileName, userCMD); 
		if(strcmp(userCMD, "t") == 0){
			int data_port_number = atoi(dportno);  // Convert C-String to int var.
			datasockfd = socket(AF_INET, SOCK_STREAM, 0); // Open new TCP connection for transfer.

			if (datasockfd < 0) {
				fprintf(stderr,"ERROR: opening socket.\n");
				exit(1);
			}
			gethostname(hostname, 1024); // Referenced: http://stackoverflow.com/questions/504810/how-do-i-find-the-current-machines-full-hostname-in-c-hostname-and-domain-info
			server = gethostbyname(hostname);
			if (server == NULL) {
				fprintf(stderr,"ERROR: no such host.\n");
				exit(1);
			}

			bzero((char *) &serv_addr2, sizeof(serv_addr2)); // Referenced: CS 344 Lecture.
			serv_addr2.sin_family = AF_INET; 

			bcopy((char *)server->h_addr, (char *)&serv_addr2.sin_addr.s_addr, server->h_length);
			serv_addr2.sin_port = htons(data_port_number); // Referenced: CS 344 Lecture.

			if (connect(datasockfd,(struct sockaddr *) &serv_addr2,sizeof(serv_addr2)) < 0) { // Referenced: CS 344 Lecture.
				fprintf(stderr,"ERROR: connecting data connection.\n");
				close(datasockfd); // Although all sockets closed server continues to listen for new incoming connections on specified port.
				close(newsockfd);
				close(sockfd);
				exit(1);
			}
			
			dataTransfer(datasockfd, command, fileName, data_port_number, hostname);
			close(datasockfd);  // Although all sockets closed server continues to listen for new incoming connections on specified port.
			close(sockfd);
		}else{
			close(newsockfd);
			close(sockfd);}
	}
	
	return 0;
}

void getRequest(int sock, char* cmd, char* fname, char *response){
	int n;
	n = read(sock,cmd,10);
	if (n < 0) {
		fprintf(stderr, "ERROR: reading from socket.\n");
		close(sock);
		exit(1);
	}

	if((strcmp(cmd, "-l") == 0)){   // Eval user input.
		n = write(sock,"ok",3);
		if (n < 0) { 
			fprintf(stderr, "ERROR: writing to socket.\n");
			close(sock);
			exit(1);
		}
		strcpy(response, "t");
	}

	else if(strcmp(cmd, "-g") == 0){ // Eval user input.
		n = write(sock,"ok",3);
		if (n < 0) { 
			fprintf(stderr, "ERROR: writing to socket.\n");
			close(sock);
			exit(1);
		}
		n = read(sock,fname,50); // Read in user's file name.
		if (n < 0) {
			fprintf(stderr, "ERROR: reading from socket.\n");
			close(sock);
			exit(1);
		}
		n = write(sock,"ok",3);
		if (n < 0) { 
			fprintf(stderr, "ERROR: writing to socket.\n");
			close(sock);
			exit(1);
		}
		strcpy(response, "t");
	}else { 	// Invalid command.
		n = write(sock,"INVALID COMMAND.",17);
		if (n < 0) { 
			fprintf(stderr, "ERROR: writing to socket.\n");
			close(sock);
			exit(1);
		}
		strcpy(response, "f");
	}
}

void dataTransfer(int sock, char* comm, char* fname, int portnum, char* hostName){
	char myDir[1024] = ""; // Buffer holds dir contents. Referenced: http://pubs.opengroup.org/onlinepubs/007908775/xsh/dirent.h.html
	DIR *dir; 	// Referenced: http://pubs.opengroup.org/onlinepubs/007908775/xsh/dirent.h.html
        struct dirent *ent;

	int n; 
	if((strcmp(comm, "-l") == 0)){
		char newLine[2] = "\n"; /* to add a newline to separate each piece of directory contents */
		printf("Transfering directory contents to on data connection on port: %d\n", portnum);

		char cwd[1024];
		getcwd(cwd, sizeof(cwd));

		if ((dir = opendir (cwd)) != NULL) {
			while ((ent = readdir (dir)) != NULL) {
				strcat(myDir, ent->d_name); 
				strcat(myDir, newLine); // Concat new line char to end of C-String.
			}

			closedir (dir);

			n = write(sock,myDir,strlen(myDir));
			if (n < 0){   // Referenced: CS 344 Lecture.
				fprintf(stderr, "ERROR: writing to Data socket: %d\n", n);
				exit(1);
			}
		}
		
	}else{
		FILE *myFD; // New file descriptor to attempt to open file if it exists.

		myFD = fopen(fname,"r");

		if(myFD==0){ 
			char badResponse[20];
			n = write(sock,"FILE NOT FOUND",15);

			if (n < 0) { 
				fprintf(stderr, "ERROR: writing to Data socket: %d\n",n);
				close(sock);
				exit(1);
			}

			n = read(sock,badResponse,20);
			if (n < 0) {
				fprintf(stderr, "ERROR: reading from socket.\n");
				close(sock);
				exit(1);
			}
		}else{
			char okResponse[20];
			n = write(sock,"Sending File",13);

			if (n < 0) { 
				fprintf(stderr, "ERROR: writing to socket: %d\n",n);
				close(sock);
				exit(1);
			}

			n = read(sock,okResponse,20);
			if (n < 0) {
				fprintf(stderr, "ERROR: reading from socket: %d\n",n);
				close(sock);
				exit(1);
			}
			printf("Sending \"%s\" to Client:%d\n", fname, portnum);
			
			while(1){ //Referenced: http://codereview.stackexchange.com/questions/43914/client-server-implementation-in-c-sending-data-files */
				unsigned char buff[1024]={0};

				int nread = fread(buff,1,1024,myFD);      
				if(nread > 0){
					write(sock, buff, nread); // Writing to client in 1024 byte portions.
				}
				if (nread < 1024){
					if (feof(myFD)){
						break; // Break from loop at EOF.
					}
					if (ferror(myFD)){
						fprintf(stderr, "ERROR: reading file.\n");
						break;
					}
				}
			}
		}
	}

	close(sock);
}

