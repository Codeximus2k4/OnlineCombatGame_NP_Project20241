#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <arpa/inet.h>
#include <ctype.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <signal.h>
#include "data_structs.cpp"

#define BUFF_SIZE 1024 // MAX UDP packet size is 1500 bytes

// Define color escape codes for colorful text
#define RESET   "\033[0m"
#define BOLD    "\033[1m"
#define RED     "\033[0;31m"
#define GREEN   "\033[0;32m"
#define YELLOW  "\033[0;33m"
#define BLUE    "\033[0;34m"
#define MAGENTA "\033[0;35m"
#define CYAN    "\033[0;36m"
#define WHITE   "\033[0;37m"

/*---------------------------------------------
Defining Player structs
-----------------------------------------------
*/


/*---------------------------------------------
Defining global variables
-----------------------------------------------
*/


/*---------------------------------------------
Defining functions
-----------------------------------------------
*/

// 
void updateListPlayer() {

}

// function to publish player information to all other clients
// - input: data from a single client that needs to be broadcasted
// dependencies: 
void broadCastData(Player player) {
    
}

/*---------------------------------------------
Main function to handle game room logic
-----------------------------------------------
*/

int gameRoom() {
    int cli_udp_port_index; // index of port from server to assign to subprocess to handle client
    pid_t pid; // test pid
    int rcvBytes, sendBytes;
    int connectfd;
    char buff[BUFF_SIZE + 1];
    struct sockaddr_in servaddr;
    struct sockaddr_in cliaddr; // list of clients addresses
    socklen_t addr_len = sizeof(struct sockaddr_in); // size of address structure, in this case size of IPv4 structure
    int SERV_PORT;
    char cli_addr[100];
    char *result; // result string to return to client
    char errorString[10000]; // actual string return to client (used in case of error)
    int sockfd;

    //Step 1: Construct socket using SOCK_STREAM (TCP)
    if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
        perror("Error constructing socket: ");
        return 0;
    }
    fprintf(stdout, GREEN "[+] Successfully created TCP socket\n" RESET);

    //Step 2: Bind address to socket
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET; // user IPv4
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY); // set server to accept connection from any network interface (IPv4 only)
    servaddr.sin_port = htons(SERV_PORT); // set port of the server

    if(bind(sockfd, (struct sockaddr *) &servaddr, sizeof(servaddr))){
        perror(RED "Error binding TCP socket: " RESET);
        return 0;
    }

    // Step 3: Listen for incoming connections 
    // backlog = 10 -> accept at most 10 connections at a time
    if(listen(sockfd, 10) < 0){
        perror(RED "Error listening on TCP socket" RESET);
        return 1;
    }

    printf(GREEN "[+] Server started. Listening on port: %d using SOCK_STREAM (TCP)\n" RESET, SERV_PORT);

    //Step 4: Accept and handle client connections
    while(1){
        // ------------------------ ACCEPT NEXT CONNECTION -----------------------------
        // accept connections waiting in the queue
        // create a new file descriptor
        connectfd = accept(sockfd, (struct sockaddr *) &cliaddr, &addr_len);
        if(connectfd < 0){
            perror(RED "Error accepting new connection from new client" RESET);
            continue;
        }
        
        // reset the buff
        memset(buff, 0, sizeof(buff));

        // receive data from this connection
        if( (rcvBytes = recv(connectfd, buff, BUFF_SIZE, 0)) == -1){
            perror(RED "Error receiving data from client: " RESET);
            continue;
        }

        buff[rcvBytes] = '\0';

        // now the last character in buff is \n -> makes the string len + 1, 
        // we need to remove this character
        if(buff[strlen(buff) - 1] == '\n') buff[strlen(buff) - 1] = '\0';


    }

    return 0;
}
