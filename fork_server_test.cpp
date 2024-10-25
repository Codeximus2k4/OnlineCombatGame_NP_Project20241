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

#define BUFF_SIZE 1024 // MAX UDP packet size is 1500 bytes
#define UDP_PORT 7070 // UPD port for transferring data

/*---------------------------------------------
Defining Player structs
-----------------------------------------------
*/

/*
Data format: data type is in string:
id|name|position_x|position_y|flip|action

- id: id of player
- name: name of player
- position_x, position_y: coordinate of player
- flip: 1 (True) or 0 (False)
- action: action of player
*/
struct Player {
    int id; // id of player
    sockaddr_in cliaddr; // IPv4 address corresponding to each player
    Player *next; // next player in the list
};

/*---------------------------------------------
Defining global variables
-----------------------------------------------
*/

int shmid; // shared memory id to use for holding list of players and their addresses
int key;
char *shm_data; 
int sockfd; // sockfd for listening on TCP on general server

/* Shared memory data structure format
# of players
player information on each line

for e.g: a room with 2 players, then shared memory data is:
2
1|127.0.0.1|55555
2|127.0.0.1|66666

*/

// define list of players
Player *players = NULL; // pointer to head of linked list of players
int playerCount = 0; // keep count of total players in the room

/*---------------------------------------------
Defining functions
-----------------------------------------------
*/

// Cleanup function to handle Ctrl+C (SIGINT)
// clean up shared memory created by the server
void signalHandler(int sig) {
    printf("\nCaught signal %d. Cleaning up and exiting...\n", sig);

    // close socket
    close(sockfd);
    printf("socketfd closed\n");

    // Detach from the shared memory
    if (shmdt(shm_data) == -1) {
        perror("shmdt failed");
        exit(1);
    }

    // Remove the shared memory
    if (shmctl(shmid, IPC_RMID, NULL) == -1) {
        perror("shmctl failed to delete shared memory");
    }

    printf("shared memory deleted\n");

    exit(0);
}

// function to clean up sockets and shared memory if an error occurs
void cleanup(int shmid, int sockfd) {
    // close socket
    close(sockfd);

    // Delete the shared memory segment
    if (shmctl(shmid, IPC_RMID, NULL) == -1) {
        perror("shmctl failed to delete shared memory");
        exit(1);
    }
}

// - make a new Player based on the information provided
// - input: all player's information
// - output: pointer to a new Player
// - dependencies: none
Player *makePlayer(int id, sockaddr_in cliaddr){
   Player *p = (Player*) malloc(sizeof(Player));
    p->id = id;
    p->cliaddr = cliaddr;

    p->next = NULL;

    return p;
}

// - add a player to a list of existing player, or create a new list if no players exist
// - input: a Player
// - output: update "players" pointer
// - dependencies: none
Player *addPlayer(Player *player){
    // if there are no players yet
    if(players == NULL){
        players = player;

        return players;
    }

    // else add to the end of the linked list
    Player *p = players;
    while(p->next != NULL){
        p = p->next;
    }

    p->next = player;

    return players;
}

// - function to serialize Player info into a string
// - input: pointer to struct Player
// - output: pointer to string contains player information
// - dependencies: none
char *serializePlayerInfo(Player *player){
    char *string = (char *) malloc(10000); // string pointer that points to string storing serialized player data
    char strnum[50]; // used for converting integer to array of char
    char *ip;
    int port;
    
    // append player id
    snprintf(string, sizeof(string), "%d", player->id);
    strcat(string, "|");

    // append player IP address
    inet_ntop(AF_INET, &player->cliaddr.sin_addr, ip, INET_ADDRSTRLEN);
    strcat(string, ip);
    strcat(string, "|");

    // append player port
    port = ntohs(player->cliaddr.sin_port);
    snprintf(strnum, sizeof(strnum), "%d", port);
    strcat(string, strnum);
    strcat(string, "|");

    return string;
}

// - function to make Player from serialized string information
// - input: serialized Player data format, sockaddr_in client address
// - output: a pointer to a new player with information filled
// - dependencies: makePlayer()
Player *unserializePlayerInfo(char information[], sockaddr_in cliaddr){
    int id; 
    char *ip;
    int port;

    char string[500]; // copy string from information so that we dont mess with original information
    strcpy(string, information);

    char *token;
    // get id
    token = strtok(string, "|");
    id = atoi(token);

    Player *p = makePlayer(id, cliaddr);

    return p;
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

// - check if a client address is already in the list of stored players
// - input: a client address
// - output: true if client is in list
//         false otherwise
// - dependencies: checkSameAddress()
bool clientInList(sockaddr_in *client){
    // if list of players is empty then false
    if(players == NULL) return false;

    Player *p = players;

    // loop through list to compare
    while(p != NULL){
        if(checkSameAddress(p->cliaddr, *client) == true){
            return true;
        }
        p = p->next;
    }

    return false;
}

// 
void updateListPlayer() {

}

// function to publish player information to all other clients
// input: data from a single client that needs to be broadcasted
// dependencies: 
void broadCastData(Player player) {
    
}

// after a "Start game" request received from client side
// this function will handle handle transferring data between client
// and corresponding subprocess
// - input: TCP socket that accepted from client, sockaddr_in of client address, string of client address, id of shared memory created 
// this function will run indefinitely until client closes connection
void handleClient(int connectfd, sockaddr_in cliaddr, char cli_addr[], int shmid) {
    // connectfd remains valid until client closes connection (recv returns 0)
    // or server uses closes(connectfd)

    int clientfd; // socket for transferring data with client (UDP)
    struct sockaddr_in servaddr;
    int status;
    char buff[BUFF_SIZE + 1];
    socklen_t addr_len = sizeof(servaddr);
    int rcvBytes, sendBytes;
    char *result;
    char *shm_cli_data; // for reading shared memory from subprocess

    // initialized server address for UDP data transfer
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET; // user IPv4
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY); // set server to accept connection from any network interface (IPv4 only)
    servaddr.sin_port = htons(UDP_PORT); // set port of the server

    // keeps trying to create a socket if fails
    do {
        clientfd = socket(PF_INET, SOCK_DGRAM, 0);
        if(clientfd == -1) {
            perror("Error creating UDP socket");
        }
    } while(clientfd == -1);

    // set this TCP and UDP sockets nonblocking (if there is no data keeps running)
    fcntl(connectfd, F_SETFL, O_NONBLOCK);
    fcntl(clientfd, F_SETFL, O_NONBLOCK);

    // bind socket (report on failure)
    status = bind(clientfd, (struct sockaddr*) &servaddr, sizeof(servaddr));
    if(status == -1) {
        perror("Error binding UDP socket to handle client data transfer");
        fprintf(stdout, "Try exiting VSC and run again\n");
        cleanup(shmid, clientfd);
    }

    printf("Subprocess created to handle client [%s:%d]\n", cli_addr, ntohs(cliaddr.sin_port));

    while(1){
        // ------------------------ CHECK CONNECTION -----------------------------
        // HANLDE TCP connection from client
        memset(buff, 0, sizeof(buff));
        rcvBytes = recv(connectfd, buff, BUFF_SIZE, 0);
        
        if (rcvBytes == 0) {
            // Client has closed the connection
            // in non-blocking mode recv returns -1 if no data is available
            // 0 if client closes connection
            printf("Client [%s:%d] has disconnected.\n", cli_addr, ntohs(cliaddr.sin_port));
            close(connectfd);
            close(clientfd);
            // Detach from the shared memory
            if (shmdt(shm_cli_data) == -1) {
                perror("shmdt failed");
                exit(1);
            }
            return;
        }
        buff[rcvBytes] = '\0';
        if(buff[strlen(buff) - 1] == '\n') buff[strlen(buff) - 1] = '\0';

        // print received client address, port and data received via TCP connection
        //printf("[%s:%d]: %s\n", cli_addr, ntohs(cliaddr.sin_port), buff);

        // ------------------------ READ DATA FROM SHARED MEMORY -----------------------------
        // point shm_data to shared memory region
        shm_cli_data = (char *) shmat(shmid, NULL, 0);
        printf("Current data in shared memory (read from subprocess):\n%s, strlen = %d\n", shm_cli_data, strlen(shm_cli_data));


        // ------------------------ UDP DATA TRANSFER -----------------------------
        // HANDLE UDP data transfer
        // clear buffer
        memset(buff, 0, sizeof(buff));

        // receive data from client (UDP)
        rcvBytes = recvfrom(clientfd, buff, BUFF_SIZE, 0, (struct sockaddr *) &cliaddr, &addr_len);
        if(rcvBytes < 0){ // do nothing if client hasnt sent anything yet
            // reading from non-blocking socket and there is no data will return -1 to rcvBytes
            //printf("Error receiving data from client [%s:%d]\n", cli_addr, ntohs(cliaddr.sin_port));
            continue;
        }
        buff[rcvBytes] = '\0';

        // now the last character in buff is \n -> makes the string len + 1, 
        // we need to remove this character
        if(buff[strlen(buff) - 1] == '\n') buff[strlen(buff) - 1] = '\0';

        // print received client address, port and data received via UDP connection
        printf("[%s:%d]: %s\n", cli_addr, ntohs(cliaddr.sin_port), buff);

        result = buff;

        // send data back to client
        sendBytes = sendto(clientfd, result, strlen(result), 0, (struct sockaddr *) &cliaddr, addr_len);
        if(sendBytes < 0){
            perror("Error sending data to client: ");
            continue;
        }
    }    
}

/*---------------------------------------------
Main function to handle server logic
-----------------------------------------------
*/

int main (int argc, char *argv[]) {
    pid_t pids[20]; // hold the list of PIDs of chlidren processes
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

    // Register the signal handler for SIGINT (Ctrl+C)
    signal(SIGINT, signalHandler);

    // check if user inputed port or not
    if(argc != 2){
        fprintf(stderr, "Usage: ./fork_server_test port_number\n");
        return 1;
    }

    // check if string is correct port number 
    if( (SERV_PORT = atoi(argv[1])) == 0){
        fprintf(stderr, "Wrong port format number\n");
        return 1;
    }

    // check if port is in range
    if(SERV_PORT < 1024 || SERV_PORT > 65535){
        fprintf(stderr, "Port should be in between 1024 and 65535\n");
        return 1;
    }

    // init shared memory
    // while shared memory allocation fails keeps trying again
    do{
        shmid = shmget(key, 1024, 0777 | IPC_CREAT);
        if(shmid < 0){
            printf("shmget failed, trying again\n");
        }
    } while(shmid < 0);

    printf("Shared memory created to store players and their addresses, shmid = %d\n", shmid);

    //Step 1: Construct socket using SOCK_STREAM (TCP)
    if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
        perror("Error constructing socket: ");
        cleanup(shmid, sockfd);
        return 0;
    }
    fprintf(stdout, "Successfully created TCP socket\n");

    //Step 2: Bind address to socket
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET; // user IPv4
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY); // set server to accept connection from any network interface (IPv4 only)
    servaddr.sin_port = htons(SERV_PORT); // set port of the server

    if(bind(sockfd, (struct sockaddr *) &servaddr, sizeof(servaddr))){
        perror("Error binding TCP socket: ");
        cleanup(shmid, sockfd);
        return 0;
    }

    // Step 3: Listen for incoming connections 
    // backlog = 10 -> accept at most 10 connections at a time
    if(listen(sockfd, 10) < 0){
        perror("Error listening on TCP socket");
        cleanup(shmid, sockfd);
        return 1;
    }

    printf("Server started. Listening on port: %d using SOCK_STREAM (TCP)\n", SERV_PORT);

    // initialize list of players as 0
    shm_data = (char *) shmat(shmid, NULL, 0);
    strcpy(shm_data, "0");

    //Step 4: Accept and handle client connections
    while(1){
        // ------------------------ READ DATA FROM SHARED MEMORY -----------------------------
        // read data from shared memory to update list of players
        shm_data = (char *) shmat(shmid, NULL, 0);
        updateListPlayer();


        // ------------------------ ACCEPT NEXT CONNECTION -----------------------------
        // accept connections waiting in the queue
        // create a new file descriptor
        connectfd = accept(sockfd, (struct sockaddr *) &cliaddr, &addr_len);
        if(connectfd < 0){
            perror("Error accepting new connection");
            continue;
        }
        
        // reset the buff
        memset(buff, 0, sizeof(buff));

        // receive data from this connection
        if( (rcvBytes = recv(connectfd, buff, BUFF_SIZE, 0)) == -1){
            perror("Error receiving data from client: ");
            continue;
        }

        buff[rcvBytes] = '\0';

        // now the last character in buff is \n -> makes the string len + 1, 
        // we need to remove this character
        if(buff[strlen(buff) - 1] == '\n') buff[strlen(buff) - 1] = '\0';

        // if client sends a "start game" message
        if(strcmp(buff, "Start game") == 0){
            // if client address is not in list of players
            if(!clientInList(&cliaddr)){
                // fork a new process to handle this client
                pid = fork();
                if(pid < 0){
                    perror("Error forking\n");
                    continue;
                }
                else if(pid == 0){
                    // child process:
                    // close connection on listening socket of parent process
                    close(sockfd);
                    
                    // store client address into cli_addr variable
                    inet_ntop(AF_INET, (void *) &cliaddr.sin_addr, cli_addr, INET_ADDRSTRLEN);

                    printf("A new connection arrived from [%s:%d], assigning subprocess for this client\n", cli_addr, ntohs(cliaddr.sin_port));

                    // handle client data transferring
                    handleClient(connectfd, cliaddr, cli_addr, shmid);

                    // kill this subprocess after client closes connection
                    return 0;
                } else{ // pid > 0
                    // parent process
                    // close connection socket descriptor of children process
                    close(connectfd);

                    // update list of players
                    playerCount++;
                    printf("Player count = %d\n", playerCount);

                    // continue accepting new connection
                    continue;
                }
            }
        }
    }

    return 0;
}
