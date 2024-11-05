#include <stdio.h>
#include "game_room.cpp"
// #include "data_structs.cpp"

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

int room_port = 10000; // port for each game room, first used port will be 10000
Room *rooms = NULL; // pointer to head of rooms created on server
Player *logged_in_players = NULL; // pointer to head of logged in players linked list
int roomCount = 0; // keep track of total rooms created

/*---------------------------------------------
Defining functions
-----------------------------------------------
*/

// 
void updateListPlayer() {

}

/*---------------------------------------------
DEFINING RESPONSE FUNCTIONS:
- note that all of the below response functions are only
responsible for sending data from server to client,
does not handle logic
-----------------------------------------------
*/

// - function to handle if client sends unauthorized request
void sendNotLoggedInResponse(int connectfd){

}

// - function to handle case client wants to register
void sendResponse1(int connectfd){
    
}

// - function to handle case client wants to login
void sendResponse2(int connectfd){

}
// - function to handle case client wants to get list room
// - input: socket descriptor to client
// - this function will send list of rooms in a serialized format (response type 3) to client, processing this data is client's job
void sendResponse3(int connectfd){
    char data[500];
    char roomInformation[500];
    int sendBytes;

    memset(roomInformation, 0, sizeof(roomInformation));

    // init data to send
    serializeRoomInformation(roomInformation, rooms);
    strcpy(data, "3");
    strcat(data, roomInformation);

    int number_of_bytes_to_send = 1 + strlen(roomInformation);

    // send to client (5 bytes)
    if( (sendBytes = send(connectfd, data, number_of_bytes_to_send, 0)) < 0){
        perror("Error");
    };
    
    // print to check
    printf(YELLOW "Number of bytes sent to client=%d\n" RESET, sendBytes);
}

// - function to handle case client wants to create room
// - input: socket descriptor connected to client, room_id, room_port
// - this function will send information of newly created room (response type 4) for client to connect
// - IMPORTANT NOTE: request type is in char, status is in char, however room_id is ASCII value
void sendResponse4(int connectfd, int room_id, int room_udp_port){
    char data[500];
    int sendBytes;

    memset(data, 0, sizeof(data));

    // init data
    data[0] = '4'; // first byte is request type
    data[1] = '1'; // second byte is status
    // set first byte of packet to length of the data 
    data[2] = (char) room_id; // third byte is room_id, note that this is ASCII value of character, e.g: '0' will be 48 in value

    // convert UDP port of room network byte (2 bytes) because there are 65536 ports
    uint16_t byte_room_port = htons(room_udp_port);

    // append these 2 bytes to response packet
    memcpy(data + 3, &byte_room_port, 2);

    // send to client (5 bytes)
    if( (sendBytes = send(connectfd, data, 5, 0)) < 0){
        perror("Error");
    };
    
    // print to check
    printf(YELLOW "Number of bytes sent to client=%d, type=%c, status=%c, room_id=%c, port=%d\n" RESET, sendBytes, data[0], data[1], data[2], room_udp_port);
}

// - function to handle case client wants to join room
void sendResponse5(int connectfd){
    
}

// - function to handle a request from client
// - input: socket descriptor connected with client, 
// - dependencies: sendResponse1, sendResponse2, sendResponse3, sendResponse4, sendResponse5
void handleRequest(int connectfd, sockaddr_in cliaddr, char cli_addr[]) {
    int recvBytes;
    char message_type;
    char buff[500];

    memset(buff, 0, sizeof(buff));

    // get first byte to determine which request
    if( (recvBytes = recv(connectfd, buff, 1, 0)) < 0){
        perror("Error");
    } else if(recvBytes == 0){
        fprintf(stdout, "Server closes connection\n");
        return;
    }
    message_type = buff[0];

    // print request type
    printf(BLUE "[+] Request type %c from [%s:%d]\n" RESET, message_type, cli_addr, ntohs(cliaddr.sin_port));

    if(message_type == '1'){
        // register request from client
        // sendResponse1(clientfd);

    } else if(message_type == '2'){
        // login request from client

    } else if(message_type == '3'){
        // get list room request from client
        sendResponse3(connectfd);

    } else if(message_type == '4'){
        // create room request from client
        int room_id = roomCount; // set room_id

        // create room and add room to list of rooms
        // note that first room_port will be 10000
        Room *room = makeRoom(room_id, room_port);
        rooms = addRoom(rooms, room);

        // fork a new game room
        int pid = fork();
        if(pid < 0){
            perror(RED "Error forking a game room\n" RESET);
            return;
        }
        else if(pid == 0){
            // child process:
            printf(BLUE "[+] Created a new game room with id = %d, udp_port used is: %d\n" RESET, roomCount, room_port, cli_addr, ntohs(cliaddr.sin_port));

            // gameRoom();
            
            // kill this subprocess after room closes
            return;
        } else{ // pid > 0
            // parent process

            // notify back to client
            sendResponse4(connectfd, room_id, room_port);

            // update room count and room port 
            roomCount++;
            room_port++;

            // exit function right after this
        }
    } else if(message_type == '5'){
        // join room request from client


    }

    return;
}

/*---------------------------------------------
Main function to handle server logic
-----------------------------------------------
*/

int main (int argc, char *argv[]) {
    int cli_udp_port_index; // index of port from server to assign to subprocess to handle client
    pid_t pid; // test pid
    int rcvBytes, sendBytes;
    int connectfd;
    struct sockaddr_in servaddr;
    struct sockaddr_in cliaddr; // list of clients addresses
    socklen_t addr_len = sizeof(struct sockaddr_in); // size of address structure, in this case size of IPv4 structure
    int SERV_PORT;
    char cli_addr[100];
    int sockfd; // sockfd for listening on TCP on general server

    // check if user inputed port or not
    if(argc != 2){
        fprintf(stderr, RED "Usage: ./server port_number\n" RESET);
        return 1;
    }

    // check if string is correct port number 
    if( (SERV_PORT = atoi(argv[1])) == 0){
        fprintf(stderr, RED "Wrong port format number\n" RESET);
        return 1;
    }

    // check if port is in range
    if(SERV_PORT < 1024 || SERV_PORT > 65535){
        fprintf(stderr, RED "Port should be in between 1024 and 65535\n" RESET);
        return 1;
    }

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

        // store client address into cli_addr variable
        inet_ntop(AF_INET, (void *) &cliaddr.sin_addr, cli_addr, INET_ADDRSTRLEN);

        printf(BLUE "[+] A new connection arrived from [%s:%d], handling this request\n" RESET, cli_addr, ntohs(cliaddr.sin_port));

        // handle this client's request
        handleRequest(connectfd, cliaddr, cli_addr);

        // close connection with this client
        close(connectfd);

        // continue handle next request   
    }

    return 0;
}
