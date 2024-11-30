#include <stdio.h>
#include "game_room.cpp"
//#include "data_structs.cpp"

#include "db_connection.cpp"
#include <libpq-fe.h> //library to connect to postgresql

//ipc libraries
#include <sys/ipc.h>
#include <sys/msg.h>

#include<pthread.h>


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

int tcp_room_port = 10000; // tcp port for each game room, first used port will be 10000
int udp_room_port = 20000; // udp port for each game room, first used port will be 20000
Room *rooms = NULL; // pointer to head of rooms created on server
int roomCount = 0; // keep track of total rooms created

/*---------------------------------------------
Defining functions
-----------------------------------------------
*/

/*---------------------------------------------
DEFINING RESPONSE FUNCTIONS:
- note that all of the below response functions are only
responsible for sending data from server to client,
does not handle logic
- data type: 
    + respond_type: char (eg: '1')
    + status: char (eg: '1')
    + id: int (eg: 1)
-----------------------------------------------
*/

// - function to handle if client sends unauthorized request
void sendNotLoggedInResponse(int connectfd){

}

// - function to handle case client wants to register
// - input: socket descriptor to client, register status (0 - fail, 1 - success)
// - this function sends response in format: [1][status]
void sendResponse1(int connectfd, int status){
    char data[5];
    int sendBytes;

    data[0] = '1';
    data[1] = status + '0';

    // send to client (2 bytes)
    if( (sendBytes = send(connectfd, data, 2, 0)) < 0){
        perror("Error");
    };
    
    // print to check
    printf(YELLOW "Number of bytes sent to client=%d\n" RESET, sendBytes);

}

// - function to handle case client wants to login
// - input: socket descriptor to client, login status (0 - fail, 1 - success), user id of the loggin client
// - this function sends response in format: [2][status][user_id], where user_id only exists when logging in successfully
void sendResponse2(int connectfd, int status, int user_id){
    char data[5];
    int sendBytes;

    data[0] = '2';
    data[1] = status + '0';
    if (user_id > 0) { //login succesfully
        data[2] = (char) user_id;
        // send to client (3 bytes)
        if( (sendBytes = send(connectfd, data, 3, 0)) < 0){
            perror("Error");
        };
    }
    else { //login fail
        // send to client (2 bytes)
        if( (sendBytes = send(connectfd, data, 2, 0)) < 0){ //if login fail, user_id field doesnt exist in message
            perror("Error");
        };
    }

    // print to check
    printf(YELLOW "Number of bytes sent to client=%d\n" RESET, sendBytes);

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
// - input: socket descriptor connected to client, room_id, tcp port of room
// - this function will send information of newly created room (response type 4) for client to connect
// - IMPORTANT NOTE: request type is in char, status is in char, however room_id is ASCII value
void sendResponse4(int connectfd, int room_id, int room_tcp_port){
    char data[500];
    int sendBytes;

    memset(data, 0, sizeof(data));

    // init data
    data[0] = '4'; // first byte is request type
    data[1] = '1'; // second byte is status
    // set first byte of packet to length of the data 
    data[2] = (char) room_id; // third byte is room_id, note that this is ASCII value of character, e.g: '0' will be 48 in value

    // convert TCP port of room network byte (2 bytes) because there are 65536 ports
    uint16_t byte_room_tcp_port = htons(room_tcp_port);
    // append these 2 bytes to response packet
    memcpy(data + 3, &byte_room_tcp_port, 2);

    // send to client (5 bytes)
    if( (sendBytes = send(connectfd, data, 5, 0)) < 0){
        perror("Error");
    };
    
    // print to check
    printf(YELLOW "Bytes sent to client=%d, type=%c, status=%c, room_id=%d, tcp_port=%d\n" RESET, sendBytes, data[0], data[1], data[2], room_tcp_port);
}

// - function to handle case client wants to join room
// - input: socket descriptor connected to client, status, room_id, tcp port of room
// - this function will send information of existed room (response type 5) for client to connect
// - IMPORTANT NOTE: request type is in char, status is in char, however room_id is ASCII value
void sendResponse5(int connectfd, int status, int room_id, int room_tcp_port){
    char data[500];
    int sendBytes;

    memset(data, 0, sizeof(data));

    // init data
    data[0] = '5'; // first byte is request type
    data[1] = status + '0'; // second byte is status
    
    // check if status = 0
    if(status == 0){
        // send to client (2 bytes only)
        if( (sendBytes = send(connectfd, data, 2, 0)) < 0){
            perror("Error");
        };

        // print to check
        printf(YELLOW "Bytes sent to client=%d, type=%c, status=%c, room_id=%d, tcp_port=%d\n" RESET, sendBytes, data[0], data[1], data[2], room_tcp_port);

        return;
    }   

    // else status = 1
    
    // set first byte of packet to length of the data 
    data[2] = (char) room_id; // third byte is room_id, note that this is ASCII value of character, e.g: '0' will be 48 in value

    // convert TCP port of room network byte (2 bytes) because there are 65536 ports
    uint16_t byte_room_tcp_port = htons(room_tcp_port);
    // append these 2 bytes to response packet
    memcpy(data + 3, &byte_room_tcp_port, 2);

    // send to client (5 bytes)
    if( (sendBytes = send(connectfd, data, 5, 0)) < 0){
        perror("Error");
    };
    
    // print to check
    printf(YELLOW "Bytes sent to client=%d, type=%c, status=%c, room_id=%d, tcp_port=%d\n" RESET, sendBytes, data[0], data[1], data[2], room_tcp_port);

}

// Thread function to handle ipc messages
// input: id of the message queue 
void *message_handler(void *arg) {
    int msgid = *(int *)arg;
    struct msqid_ds buf;

    while (1) {
        //--------------------IPC-------------------
        // Get the status of the message queue and store into buf
        if (msgctl(msgid, IPC_STAT, &buf) == -1) {
            perror("msgctl");
            exit(1);
        }

        // Check if the queue is empty
        if (buf.msg_qnum != 0) { //the queue is not empty
            // receive message from child process
            struct ipc_msg message;

            // Receive the message
            if (msgrcv(msgid, &message, sizeof(message.text), 1, 0) == -1) {
                perror("msgrcv failed");
                exit(1);
            }

            printf("Parent: Message receive from child. %s\n", message.text);

            //update room status
            int room_id_updated = message.text[0] - '0';
            Room *room_updated = findRoomById(rooms, room_id_updated); //find room
            room_updated->total_players = message.text[1] - '0'; //update num of players in room
            room_updated->started = message.text[2] - '0'; //update started status
            printf("Total numbeer of player in room %d is %d\n", room_id_updated, room_updated->total_players);
        }
    }
}


// - function to handle a request from client
// - input: socket descriptor connected with client, users db connection, the id of the message queue
// - dependencies: sendResponse1, sendResponse2, sendResponse3, sendResponse4, sendResponse5
void handleRequest(int connectfd, sockaddr_in cliaddr, char cli_addr[], PGconn *conn, int msgid) {
    int recvBytes;
    char message_type;
    char buff[500];

    memset(buff, 0, sizeof(buff));

    // get first byte to determine which request
    if( (recvBytes = recv(connectfd, buff, 1, 0)) < 0){
        perror("Error");
    } else if(recvBytes == 0){
        fprintf(stdout, "Client closes connection\n");
        return;
    }
    message_type = buff[0];

    // print request type
    printf(BLUE "[+] Request type %c from [%s:%d]\n" RESET, message_type, cli_addr, ntohs(cliaddr.sin_port));

    if(message_type == '1'){
        // register request from client
        char username[200];
        int username_length;
        char password[200];
        int password_length;

        //get username_length
        if( (recvBytes = recv(connectfd, buff, 1, 0)) < 0){
            perror("Error");
        } else if(recvBytes == 0){
            fprintf(stdout, "Client closes connection\n");
            return;
        }

        username_length = buff[0];

        // check if username string is empty
        if(username_length != 0){
            // now extract username
            if( (recvBytes = recv(connectfd, buff, username_length, 0)) < 0){
                perror("Error");
            } 
            buff[recvBytes] = '\0';
            strcpy(username, buff);
        }

        //get password_length
        if( (recvBytes = recv(connectfd, buff, 1, 0)) < 0){
            perror("Error");
        } else if(recvBytes == 0){
            fprintf(stdout, "Client closes connection\n");
            return;
        }

        password_length = buff[0];

        // check if password string is empty
        if(password_length != 0){
            // now extract password
            if( (recvBytes = recv(connectfd, buff, password_length, 0)) < 0){
                perror("Error");
            } 
            buff[recvBytes] = '\0';
            strcpy(password, buff);
        }

        int status;

        if (add_user(username, password, conn)) { //register succesfully
            status = 1;
        }
        else status = 0; //register fail
        // send response back to client
        sendResponse1(connectfd, status);
        
    } else if(message_type == '2'){
        // login request from client
        char username[200];
        int username_length;
        char password[200];
        int password_length;

        //get username_length
        if( (recvBytes = recv(connectfd, buff, 1, 0)) < 0){
            perror("Error");
        } else if(recvBytes == 0){
            fprintf(stdout, "Client closes connection\n");
            return;
        }

        username_length = buff[0];

        // check if username string is empty
        if(username_length != 0){
            // now extract username
            if( (recvBytes = recv(connectfd, buff, username_length, 0)) < 0){
                perror("Error");
            } 
            buff[recvBytes] = '\0';
            strcpy(username, buff);
        }

        //get password_length
        if( (recvBytes = recv(connectfd, buff, 1, 0)) < 0){
            perror("Error");
        } else if(recvBytes == 0){
            fprintf(stdout, "Client closes connection\n");
            return;
        }

        password_length = buff[0];

        // check if password string is empty
        if(password_length != 0){
            // now extract password
            if( (recvBytes = recv(connectfd, buff, password_length, 0)) < 0){
                perror("Error");
            } 
            buff[recvBytes] = '\0';
            strcpy(password, buff);
        }

        
        int status;
        int user_id;

        if (check_user(username, password, conn)) { //login succesfully
            status = 1;
            user_id = get_user_id(username, conn);

            // send response back to client
            sendResponse2(connectfd, status, user_id);
        }
        else { //login fail
            status = 0; 
            user_id = -1;
            // send response back to client
            sendResponse2(connectfd, status, user_id);
        }

        // update list of players 

    } else if(message_type == '3'){
        // get list room request from client
        sendResponse3(connectfd);

    } else if(message_type == '4'){
        // create room request from client
        int room_id = roomCount; // set room_id

        // get player id
        if( (recvBytes = recv(connectfd, buff, 1, 0)) < 0){
            perror("Error");
        } else if(recvBytes == 0){
            fprintf(stdout, "Client closes connection\n");
            return;
        }
        int player_id = buff[0];

        // create room and add room to list of rooms
        // note that first tcp_udp_room will be 10000 udp_room_port will be 20000
        Room *room = makeRoom(room_id, tcp_room_port, udp_room_port);
        rooms = addRoom(rooms, room);

        // fork a new game room
        int pid = fork();
        if(pid < 0){
            perror(RED "Error forking a game room\n" RESET);
            return;
        }
        else if(pid == 0){
            // child process:
            // close connection on this client
            close(connectfd);

            printf(BLUE "[+] Created a new game room with id = %d, tcp_port is: %d, udp_port is: %d\n" RESET, roomCount, tcp_room_port, udp_room_port, cli_addr, ntohs(cliaddr.sin_port));
            
            // ------------------------ GAME ROOM SERVER -----------------------------
            // now game room server is created, game room server will listen
            // on another socket with another port separated from main server
            gameRoom(room_id, tcp_room_port, udp_room_port, msgid);


            
            // kill this subprocess after room closes
            exit(0);
        } else{ // pid > 0
            // parent process

            // notify back to client
            sendResponse4(connectfd, room_id, tcp_room_port);
            

            // update room count and room ports
            roomCount++;
            tcp_room_port++;
            udp_room_port++;

            // exit function right after this
        }
    } else if(message_type == '5'){
        // join room request from client

        // get player id
        if( (recvBytes = recv(connectfd, buff, 1, 0)) < 0){
            perror("Error");
        } else if(recvBytes == 0){
            fprintf(stdout, "Client closes connection\n");
            return;
        }
        int player_id = buff[0];

        // get room id
        if( (recvBytes = recv(connectfd, buff, 1, 0)) < 0){
            perror("Error");
        } else if(recvBytes == 0){
            fprintf(stdout, "Client closes connection\n");
            return;
        }
        int room_id = buff[0] - '0';

        // check if client can join this room
        Room *room = findRoomById(rooms, room_id);
        int status;
        if(room->total_players >= 4){
            status = 0;
        } else {
            status = 1;
        }

        // if player can join
        if(status == 1){
            // send response back to client
            sendResponse5(connectfd, status, room_id, tcp_room_port);
        } else {
            // player cannot join because room full

            // send response back to client
            sendResponse5(connectfd, status, room_id, tcp_room_port);
        }
    }

    return;
}

/*---------------------------------------------
Main function to handle server logic
-----------------------------------------------
*/

int main (int argc, char *argv[]) {
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

    //connect to users db
    PGconn *conn = connect_db();

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

    //-----------handle ipc-------------------------
    int msgid;
    pthread_t thread_id;

    // Generate a unique key
    int key = ftok("progfile", 65);

    // Create or access a message queue
    msgid = msgget(key, 0666 | IPC_CREAT);

    if (msgid == -1) {
        perror("Failed to create message queue");
        exit(EXIT_FAILURE);
    }

    printf("Message queue created with ID: %d\n", msgid);

    // Create a thread to handle ipc messages
    if (pthread_create(&thread_id, NULL, message_handler, (void *)&msgid) != 0) {
        perror("Failed to create thread");
        msgctl(msgid, IPC_RMID, NULL);  // Remove message queue
        exit(EXIT_FAILURE);
    }
    

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
        handleRequest(connectfd, cliaddr, cli_addr, conn, msgid);

        // close connection with this client
        close(connectfd);

        // continue handle next request   
    }

    close_db(conn);

    // Wait for the thread to finish
    pthread_join(thread_id, NULL);

    // Clean up the message queue
    if (msgctl(msgid, IPC_RMID, NULL) == -1) {
        perror("Failed to remove message queue");
        exit(EXIT_FAILURE);
    }

    return 0;
}
