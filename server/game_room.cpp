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
#include <errno.h> // use of errno variable
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
Defining global variables
-----------------------------------------------
*/
Player *players = NULL; // list of players in this room

/*---------------------------------------------
Defining functions
-----------------------------------------------
*/

// function to publish player information to all other clients
// - input: 
// dependencies: 
void broadCastData(Player *players) {
    
}

// - function to check if 2 sockaddr_in is the same
// - input: sockaddr_in of client1 and client2
// - output: true if 2 clients have same address:port
//         false otherwise
// - dependencies: none
bool checkSameAddress(sockaddr_in cli1, sockaddr_in cli2){
    // check same IP
    if(cli1.sin_addr.s_addr == cli2.sin_addr.s_addr){
        // if also same port
        if(cli1.sin_port == cli2.sin_port) return true;
    }

    return false;
}

// - function to check which player (stored server-side) corresponds to the address provided
// - input: sockaddr_in of client
// - output: player_id corresponds to the client address, -1 if not found
int findPlayerIdByAddress(sockaddr_in cliaddr){   
    Player *p = players;

    while(p != NULL){
        if(checkSameAddress(p->cliaddr, cliaddr)) return p->id;
    }

    return -1;
}


/*---------------------------------------------
Main function to handle game room logic
-----------------------------------------------
*/

// - function for each game room
// - input: tcp port, udp port for this game room
// - output: 0 when room closes, 1 on error creating room
// - will run until the room closes
int gameRoom(int TCP_SERV_PORT, int UDP_SERV_PORT) {
    pid_t pid; // test pid
    int recvBytes, sendBytes;
    int connectfd;
    char buff[BUFF_SIZE + 1];
    struct sockaddr_in servaddr;
    struct sockaddr_in cliaddr; // list of clients addresses
    socklen_t addr_len = sizeof(struct sockaddr_in); // size of address structure, in this case size of IPv4 structure
    char cli_addr[100];
    int sockfd; // sockfd to handle connection from client
    int clientfd; // handle udp data transfer from clients
    int status;
    char data_client[100][500];
    int sent = 0; // check if we received data from at least one of the clients


    // ------------------------ CONSTRUCT TCP SOCKET -----------------------------
    // Construct tcp socket using SOCK_STREAM (TCP)
    if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
        perror("Error constructing socket: ");
        return 1;
    }
    fprintf(stdout, GREEN "[+] Successfully created TCP socket for game room with tcp_port: %d\n" RESET, TCP_SERV_PORT);

    // set TCP socket to non blocking
    int flags = fcntl(sockfd, F_GETFL, 0);      // Get current flags
    fcntl(sockfd, F_SETFL, flags | O_NONBLOCK); // Set to non-blocking mode

    // Bind address to socket
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET; // user IPv4
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY); // set server to accept connection from any network interface (IPv4 only)
    servaddr.sin_port = htons(TCP_SERV_PORT); // set port of the server

    if(bind(sockfd, (struct sockaddr *) &servaddr, sizeof(servaddr))){
        perror(RED "Error binding TCP socket: " RESET);
        return 1;
    }

    // Listen for incoming connections 
    // backlog = 10 -> accept at most 10 connections at a time
    if(listen(sockfd, 10) < 0){
        perror(RED "Error listening on TCP socket: " RESET);
        return 1;
    }

    // ------------------------ CONSTRUCT UDP SOCKET -----------------------------
    // initialized server address for UDP data transfer
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET; // user IPv4
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY); // set server to accept connection from any network interface (IPv4 only)

    // assign UDP port of server 
    servaddr.sin_port = htons(UDP_SERV_PORT); // set port of the server

    // keeps trying to create a socket if fails
    do {
        clientfd = socket(PF_INET, SOCK_DGRAM, 0);
        if(clientfd == -1) {
            perror(RED "Error creating UDP socket" RED);
        }
    } while(clientfd == -1);

    // set this UDP socket nonblocking (if there is no data keeps running)
    flags = fcntl(clientfd, F_GETFL, 0);      // Get current flags
    fcntl(clientfd, F_SETFL, flags | O_NONBLOCK); // Set to non-blocking mode

    // bind socket (report on failure)
    status = bind(clientfd, (struct sockaddr*) &servaddr, sizeof(servaddr));
    if(status == -1) {
        perror(RED "Error binding UDP socket to handle client data transfer" RESET);
        fprintf(stdout, "Try exiting VSC and run again\n");
        return 1;
    }

    // ------------------------ NOTIFY -----------------------------
    printf(GREEN "[+] Game room server started. TCP port: %d, UDP port: %d\n" RESET, TCP_SERV_PORT, UDP_SERV_PORT);

    // Accept and handle client connections
    while(1){
        // reset data
        sent = 0; 
        memset(data_client, 0, sizeof(data_client));

        // ------------------------ CHECK NEW CONNECTION -----------------------------
        // accept connection from client wants to connect to the room
        // create a new file descriptor
        connectfd = accept(sockfd, (struct sockaddr *) &cliaddr, &addr_len);
        
        // check if there is connection request
        if (connectfd == -1) {
            // in non-blocking mode, connectfd returns -1 on no connection or error
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                // No connection is available right now

                // continue;

            } else {
                // Handle other errors
                perror("Accept error");

            }
        } else if(connectfd > 0){ // there is a new connection request
            // store client address into cli_addr variable
            inet_ntop(AF_INET, (void *) &cliaddr.sin_addr, cli_addr, INET_ADDRSTRLEN);

            // get player_id from the client
            if( (recvBytes = recv(connectfd, buff, 1, 0)) < 0){
                perror("Error");
            }
            int player_id = buff[0];

            // add this client to list of players
            Player *p = makePlayer(player_id, cliaddr);
            players = addPlayerToLoginList(players, p);

            // close connection to this client
            close(connectfd);
        }

        // ------------------------ RECEIVE DATA AND TRANSFER -----------------------------
        // receive data from list of clients connected
        // receive data from first client (note that this can be any client,
        // we need to check IP where the data is from) (UDP)
        recvBytes = recvfrom(clientfd, buff, BUFF_SIZE, 0, (struct sockaddr *) &cliaddr, &addr_len);
        if(recvBytes == -1){ 
            if(errno == EAGAIN || errno == EWOULDBLOCK){
                // do nothing if client hasnt sent anything yet

            } else{
                // reading from non-blocking socket and there is no data will return -1 to rcvBytes
                //printf("Error receiving data from client [%s:%d]\n", cli_addr, ntohs(cliaddr.sin_port));

            }
        } else { // if we receive data from client
            // flag sent variable since we received data
            sent = 1; 

            buff[recvBytes] = '\0';
            if(buff[strlen(buff) - 1] == '\n') buff[strlen(buff) - 1] = '\0';

            // check which player corresponds to this data
            int check = findPlayerIdByAddress(cliaddr);

            if(check > 0){
                // copy data into corresponding player data
                strcpy(data_client[check], buff);

            }
        }

        // receive data from second client (UDP)
        recvBytes = recvfrom(clientfd, buff, BUFF_SIZE, 0, (struct sockaddr *) &cliaddr, &addr_len);
        if(recvBytes == -1){ 
            if(errno == EAGAIN || errno == EWOULDBLOCK){
                // do nothing if client hasnt sent anything yet

            } else{
                // reading from non-blocking socket and there is no data will return -1 to rcvBytes
                //printf("Error receiving data from client [%s:%d]\n", cli_addr, ntohs(cliaddr.sin_port));

            }
        } else { // if we receive data from client
            // flag sent variable since we received data
            sent = 1; 

            buff[recvBytes] = '\0';
            if(buff[strlen(buff) - 1] == '\n') buff[strlen(buff) - 1] = '\0';

            // check which player corresponds to this data
            int check = findPlayerIdByAddress(cliaddr);

            if(check > 0){
                // copy data into corresponding player data
                strcpy(data_client[check], buff);

            }
        }

        // receive data from third client (UDP)
        recvBytes = recvfrom(clientfd, buff, BUFF_SIZE, 0, (struct sockaddr *) &cliaddr, &addr_len);
        if(recvBytes == -1){ 
            if(errno == EAGAIN || errno == EWOULDBLOCK){
                // do nothing if client hasnt sent anything yet

            } else{
                // reading from non-blocking socket and there is no data will return -1 to rcvBytes
                //printf("Error receiving data from client [%s:%d]\n", cli_addr, ntohs(cliaddr.sin_port));

            }
        } else { // if we receive data from client
            // flag sent variable since we received data
            sent = 1; 

            buff[recvBytes] = '\0';
            if(buff[strlen(buff) - 1] == '\n') buff[strlen(buff) - 1] = '\0';

            // check which player corresponds to this data
            int check = findPlayerIdByAddress(cliaddr);

            if(check > 0){
                // copy data into corresponding player data
                strcpy(data_client[check], buff);

            }
        }

        // receive data from fourth client (UDP)
        recvBytes = recvfrom(clientfd, buff, BUFF_SIZE, 0, (struct sockaddr *) &cliaddr, &addr_len);
        if(recvBytes == -1){ 
            if(errno == EAGAIN || errno == EWOULDBLOCK){
                // do nothing if client hasnt sent anything yet

            } else{
                // reading from non-blocking socket and there is no data will return -1 to rcvBytes
                //printf("Error receiving data from client [%s:%d]\n", cli_addr, ntohs(cliaddr.sin_port));

            }
        } else { // if we receive data from client
            // flag sent variable since we received data
            sent = 1; 

            buff[recvBytes] = '\0';
            if(buff[strlen(buff) - 1] == '\n') buff[strlen(buff) - 1] = '\0';

            // check which player corresponds to this data
            int check = findPlayerIdByAddress(cliaddr);

            if(check > 0){
                // copy data into corresponding player data
                strcpy(data_client[check], buff);

            }
        }

        // check 'sent' flag to determine to broadcast data or not
        if(sent == 0){
            // if we havent received data from any of the clients

            continue;
        }
        // else:     
        // now data from 4 clients will be stored in corresponding data_client[player_id] array


        // process logic of game
        

        // broadcast data back to client
    }

    return 0;
}
