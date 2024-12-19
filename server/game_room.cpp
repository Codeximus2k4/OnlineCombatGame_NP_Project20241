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

//ipc libraries
#include <sys/ipc.h>
#include <sys/msg.h>

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

// function to get a TCP listening socket
// - input: server port in string type
// - output: file descriptor of listening socket
int get_listener_socket(char SERV_PORT[BUFF_SIZE + 1]){
    int listener; // listening socket descriptor
    int yes = 1; // for setsockopt() SO_REUSEADDR, below
    int rv;

    struct addrinfo hints, *ai, *p;

    // get us a socket and bind it 
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET; // allow IPv4 only
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    if( (rv = (getaddrinfo(NULL, SERV_PORT, &hints, &ai)) != 0)){
        fprintf(stderr, "pollserver: %s\n", gai_strerror(rv));
    }

    for(p = ai; p != NULL; p = p->ai_next){
        listener = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if(listener < 0){
            continue;
        }

        // lose the pesky "address already in use" error message
        setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));

        if(bind(listener, p->ai_addr, p->ai_addrlen) < 0){
            close(listener);
            continue;
        }

        break;
    }

    freeaddrinfo(ai); // all done with this

    // if we didn't get any bound socket
    if(p == NULL){
        return -1;
    }

    // listen
    if(listen(listener, 10) == -1){
        return -1;
    }

    return listener;
}

// get sockaddr, IPv4 or IPv6
// (used in select())
void *get_in_addr(struct sockaddr *sa){
    if(sa->sa_family == AF_INET){
        return &(((struct sockaddr_in*) sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*) sa)->sin6_addr);
}

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
// - dependencies: checkSameAddress()
int findPlayerIdByAddress(sockaddr_in cliaddr){   
    Player *p = players;

    while(p != NULL){
        if(checkSameAddress(p->cliaddr, cliaddr)) return p->id;
    }

    return -1;
}

// - function to send response to client when they send request type 6
// - input: socket descriptor connected to client, status (if client can successfully joined game room server)
// - IMPORTANT NOTE: request type is in char, status is in char, however room_id is ASCII value
void sendResponse6(int connectfd, int status){
    char data[500];
    int sendBytes;

    memset(data, 0, sizeof(data));

    // init data
    data[0] = '6'; // first byte is response type
    data[1] = status + '0'; // second byte is status

    // send to client (2 bytes)
    if( (sendBytes = send(connectfd, data, 2, 0)) < 0){
        perror(RED "Error inside sendResponse6()" RESET);
    };
    
    // print to check
    printf(YELLOW "Bytes sent to client=%d, type=%c, status=%c\n" RESET, sendBytes, data[0], data[1]);
}

// function to handle data when connected clients want to send something to server (for e.g: player ready state)
// - handle request type 7 from clients
void handleConnectedClients(int clientfd, char buff[BUFF_SIZE + 1]) {
    char key;
    char payload[BUFF_SIZE + 1];
    char packet[BUFF_SIZE + 1];
    int sendBytes;
    int recvBytes;

    // get opcode 
    int opcode = buff[0];

    // get user_id
    if( (recvBytes = recv(clientfd, buff, 1, 0)) < 0){
        perror(RED "Error inside handleConnectedClients() getting `user_id`" RESET);
    } else if(recvBytes == 0){
        fprintf(stdout, "Client closes connection\n");
        return;
    }
    int player_id = buff[0];

    // get ready
    if( (recvBytes = recv(clientfd, buff, 1, 0)) < 0){
        perror(RED "Error inside handleConnectedClients() getting `ready`" RESET);
    } else if(recvBytes == 0){
        fprintf(stdout, "Client closes connection\n");
        return;
    }
    int ready = buff[0];

    // update internal linked list
    Player *correspondingPlayer = findPlayerInRoomById(players, player_id);
    correspondingPlayer->ready = ready;
}

// - function to send player id 
// - input: socket descriptor connected to client, the player
// - IMPORTANT NOTE: request type is in char, num_player is in char
void sendResponse8(int connectfd, Player *player){
    char data[500];
    int sendBytes;

    memset(data, 0, sizeof(data));

    // init data
    data[0] = '8'; // first byte is response type
    data[1] = player->id; 

    // send to client (2 bytes)
    if( (sendBytes = send(connectfd, data, 2, 0)) < 0){
        perror(RED "Error inside sendResponse8()" RESET);
    };
    
    // print to check
    printf(YELLOW "Bytes sent to client=%d, type=%c, player_id=%c\n" RESET, sendBytes, data[0], data[1] + '0');
}


/*---------------------------------------------
Main function to handle game room logic
-----------------------------------------------
*/

// - function for each game room
// - input: room_id, tcp port, udp port for this game room, msgid: id of the message queue used for ipc
// - output: 0 when room closes, 1 on error creating room
// - will run until the room closes
int gameRoom(int room_id, int TCP_SERV_PORT, int UDP_SERV_PORT, int msgid) {
    fd_set master; // master file descriptor list 
    fd_set read_fds; // temp file descriptor list for select()
    int connectfd;
    char buff[BUFF_SIZE + 1];
    struct addrinfo hints, *ai, *p;
    char cli_addr[100];
    int sockfd; // sockfd to handle connection from client
    int clientfd; // handle udp data transfer from clients
    char data_client[100][500];
    int sent = 0; // check if we received data from at least one of the clients
    int started = 0; 
    int rv; // use for checking status of functions return
    int listener; // listening socket descriptor
    int newfd; // newly accepted socket descriptor
    int fdmax; // maximum file descriptor number
    struct sockaddr_in remoteaddr; // client address
    socklen_t addrlen;
    char remoteIP[INET6_ADDRSTRLEN];
    int nbytes;
    int total_players = 0;

    // convert port to string
    char CHAR_TCP_PORT[BUFF_SIZE + 1];
    sprintf(CHAR_TCP_PORT, "%d", TCP_SERV_PORT);


    // ------------------------ WAITING PERIOD -----------------------------
    // use select() to handle new connections

    // get a TCP listening socket
    listener = get_listener_socket(CHAR_TCP_PORT);

    // add the listener to the master set
    FD_SET(listener, &master);

    // keep track of the biggest file descriptor
    fdmax = listener; // so far it's this one

    // notify 
    printf(YELLOW "Gameroom Server [%d] listening on %s\n" RESET, room_id, CHAR_TCP_PORT);

    // main loop
    for(;;){
        read_fds = master; // copy master set so we don't modify it, we need original version of this set

        // note that select() hanldes from file descriptor 0 -> fdmax (if arguments are provided as below)
        // however, select() will only handle those that have been added to `read_fds`
        if(select(fdmax + 1, &read_fds, NULL, NULL, NULL) == -1){
            perror(RED "Error in waiting room, select period" RESET);
            exit(4);
        }

        // run through the existing connections looking for data to read
        for(int i = 0; i <= fdmax; i++){
            memset(buff, 0, sizeof(buff));

            // if we have one (can be new connection or data from connected clients)
            if(FD_ISSET(i, &read_fds)){
                // printf("socket %d has sth to talk\n", i);

                if(i == listener){
                    // handle new connections
                    addrlen = sizeof(remoteaddr);
                    newfd = accept(listener, (struct sockaddr*) &remoteaddr, &addrlen);

                    if(newfd == -1){
                        perror(RED "Error in waiting room, accept" RESET);
                    } else {
                        // notify
                        printf(YELLOW "Gameroom [%d]: a new connection from %s:%d on socket %d\n" RESET, 
                            room_id,
                            inet_ntop(remoteaddr.sin_family, 
                                get_in_addr((struct sockaddr*) &remoteaddr), 
                                remoteIP, 
                                INET6_ADDRSTRLEN
                            ), 
                            ntohs(remoteaddr.sin_port),
                            newfd
                        );

                        // receive data from this new connection to check if it is authorized to join game room
                        if( (nbytes = recv(newfd, buff, 1, 0)) <= 0){ // get one byte only (which is opcode, so we can process)
                            // got error or connection closed by client
                            if(nbytes == 0){
                                // connection closed
                                printf(YELLOW "Gameroom [%d]: socket %d hung up\n" RESET, room_id, i);
                            } else {
                                perror(RED "Error in waiting room, receive data" RESET);
                            }
                        } else { // check if we can accept this new connection or not 
                            int can_join = 1;

                            // get user_id
                            if( (nbytes = recv(newfd, buff, 1, 0)) < 0){
                                perror(RED "Error in waiting room, new connection" RESET);
                            } else if(nbytes == 0){
                                fprintf(stdout, "Client closes connection\n");
                                //return;
                                continue;
                            }
                            int player_id = buff[0];

                            // check total players
                            total_players = countPlayerInRoom(players);

                            if(total_players >= 4) {
                                can_join = 0;
                            }

                            // if user can join
                            if(can_join == 1){
                                FD_SET(newfd, &master); // add to master set

                                if(newfd > fdmax){
                                    // keep track of the max
                                    fdmax = newfd;
                                } 

                                // make a new player
                                Player *newPlayer = makePlayer(player_id, newfd, remoteaddr);

                                // add this player to list
                                players = addPlayerToPlayersInRoom(players, newPlayer);

                                // notify server-side
                                printf(YELLOW "Connection from %s:%d on socket %d has successfully joined game room [%d]\n" RESET, 
                                    inet_ntop(remoteaddr.sin_family, 
                                        get_in_addr((struct sockaddr*) &remoteaddr), 
                                        remoteIP, 
                                        INET6_ADDRSTRLEN
                                    ), 
                                    ntohs(remoteaddr.sin_port),
                                    newfd,
                                    room_id
                                );

                                // send message to main server about this change 
                                ipc_msg message;
                                
                                serializeIpcMsg(&message, room_id, players);
                                
                                // Send the message
                                if (msgsnd(msgid, &message, sizeof(message.text), 0) == -1) {
                                    perror(RED "Error, msgsnd failed" RESET);
                                    exit(1);
                                }
                                printf("Child: Message sent to parent. %s\n", message.text);
                                
                            } else {
                                // if player cannot join (due to some constraints)

                                // notify server-side
                                printf(YELLOW "Connection from %s:%d on socket %d failed to join game room [%d]\n" RESET, 
                                    inet_ntop(remoteaddr.sin_family, 
                                        get_in_addr((struct sockaddr*) &remoteaddr), 
                                        remoteIP, 
                                        INET6_ADDRSTRLEN
                                    ), 
                                    ntohs(remoteaddr.sin_port),
                                    newfd,
                                    room_id
                                );
                            }

                            // send response back to client
                            sendResponse6(newfd, can_join);
                        }
                    }
                } else {
                    // handle data from connected clients (players who have already successfully connected to game room)
                    if( (nbytes = recv(i, buff, 1, 0)) <= 0){ // get one byte only (which is opcode, so we can process)
                        // got error or connection closed by client (kick this client out of the game room)

                        if(nbytes == 0){
                            // connection closed
                            printf(GREEN "gameroom server [%d]: socket %d hung up\n" RESET, room_id, i);
                        } else {
                            perror(RED "Error in waiting room, receive data from connected clients" RESET);
                        }

                        close(i); // close this socket
                        FD_CLR(i, &master); // remove from master set

                        // find the player corresponding to this socket descriptor
                        Player *toRemovePlayer = findPlayerInRoomBySocketDescriptor(players, i);

                        // remove this player from linked list
                        players = removePlayerFromListPlayers(players, toRemovePlayer);

                        // send message to main server about this change 
                        ipc_msg message;
                        
                        serializeIpcMsg(&message, room_id, players);

                        // Send the message
                        if (msgsnd(msgid, &message, sizeof(message.text), 0) == -1) {
                            perror(RED "Error in waiting room, msgsnd failed" RESET);
                            exit(1);
                        }
                        printf("Child: Message sent to parent. %s\n", message.text);

                    } else {
                        // if we got some data from connected clients (i.e. handling ready state of players), process
                        
                        handleConnectedClients(i, buff);
                    }
                } // END handle data from client
                
                //send response 8 to all players in the room
                Player *p = players;
                while (p != NULL) {
                    int fd = p->socket_descriptor;
                    sendResponse8(fd, p);
                    p = p->next;
                }

            } // END got new incoming connections
        } // END looping through file descriptors
    } // END for(;;)

    
    // ------------------------ INGAME PERIOD -----------------------------
    // not implemented

    return 0;
}
