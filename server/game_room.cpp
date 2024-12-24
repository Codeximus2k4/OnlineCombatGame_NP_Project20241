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
#include<pthread.h>
#include<time.h>

#include "db_connection.cpp"
#include <libpq-fe.h> //library to connect to postgresql

//ipc libraries
#include <sys/ipc.h>
#include <sys/msg.h>

#include "data_structs.cpp"


#define TICK_RATE 25
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
int check_gameStart_condition(Player *head, int total_player_count)
{
    Player *p = head;
    int readyPlayerCount = 0;

    if(p == NULL || total_player_count==0) return 0;

    while(p != NULL){
        if (p->ready) readyPlayerCount++;
        p = p->next;
    }

    if (readyPlayerCount== total_player_count) 
    {
        return 1;
    }
    else return 0;    
}

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

// - function to update score of a specific player to database
// - input: player_id, score of the previous match (this will be augmented to `score` column in database)
// - output: 1 on successful, 0 on failure
// - dependencies
// - note that both score and unmber of games_played column in database of this player will get updated
int updateScore(int player_id, int score){
    // connect to users db
    PGconn *conn = connect_db();

    // update player's statistics (games played, score) to database
    int status = update_player_score(conn, player_id, score);

    // close database connection
    close_db(conn);

    return status;
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

int sent_UDP_port(Player *head, int udp_port)
{
    // implementing non-blocking send using select
    fd_set writefds;
    struct timeval timeout; //timeout for each client is 1 second
    timeout.tv_sec = 1;
    timeout.tv_usec= 0;
    char message[3];
    message[0]='7';
    message[1]= udp_port/256;
    message[2]= udp_port%256; 
    int finished_sending = 1;
    //printf("%d",countPlayerInRoom(head));
    // first we check the writability of each socket first
    for (Player *p=head; p!=NULL;p=p->next)
    {
        int port_bytes_sent =  p->udp_port_byte_sent;
        int tcp_client_sockfd =  p->socket_descriptor;
        if (port_bytes_sent==3) continue;
        else
        {
            finished_sending = 0;
            int bytes_sent = send(tcp_client_sockfd, message+port_bytes_sent,3 - port_bytes_sent, 0);
            if (bytes_sent<0)
            {
                char err_message[80];
                sprintf(err_message,"Error with Room message type 7 at socket %d:",tcp_client_sockfd);
                //perror(err_message);
            }
            else p->udp_port_byte_sent+=bytes_sent;
            printf("%d\n",bytes_sent);
            if (p->udp_port_byte_sent ==3) finished_sending=1;
        }
    }
    //printf("%d\n",finished_sending);
    return finished_sending;

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

    // print log
    printf("connected client with socket %d sendt request type 7\n", clientfd);

    // get opcode 
    int opcode = buff[0];

    // get user_id
    if( (recvBytes = recv(clientfd, buff, 1, 0)) < 0){
        perror(RED "Error inside handleConnectedClients() getting `user_id`" RESET);
        return;
    } else if(recvBytes == 0){
        fprintf(stdout, "Client closes connection\n");
        return;
    }
    int player_id = buff[0];

    // get ready
    if( (recvBytes = recv(clientfd, buff, 1, 0)) < 0){
        perror(RED "Error inside handleConnectedClients() getting `ready`" RESET);
        return;
    } else if(recvBytes == 0){
        fprintf(stdout, "Client closes connection\n");
        return;
    }
    int ready = buff[0];

    // update internal linked list
    Player *correspondingPlayer = findPlayerInRoomById(players, player_id);
    correspondingPlayer->ready = ready;
    printf("Player with id %d has ready status %d\n",correspondingPlayer->id, correspondingPlayer->ready);
}

// - function to send players in the room in4 to clients
// - input: socket descriptor connected to client, the head of the player list
// - IMPORTANT NOTE: request type is in char, num_player is in char
void sendResponse8(int connectfd, Player *players){
    char data[500];
    int sendBytes;

    memset(data, 0, sizeof(data));

    int data_length = serializePlayersInRoomInformation(data, players);

    printf("Data length: %d\n", data_length);
    // send to client (2 bytes)
    if( (sendBytes = send(connectfd, data, data_length, 0)) < 0){
        perror(RED "Error inside sendResponse8()" RESET);
    };
    
    // print to check
    printf(YELLOW "Bytes sent to client=%d, type=%c\n" RESET, sendBytes, data[0]);
}


// - function to handle listening data from client using thread
// - input: udp_server_socket that is used for communicating with clients
void *listenFromClients(void *arg){
    // get udp_server_socket
    int udp_server_socket = *(int *) arg;

    // init other data
    char buff[BUFF_SIZE + 1];
    int rcvBytes;
    sockaddr_in clientAddr;
    socklen_t len = sizeof(clientAddr);


    // detach this thread so that it terminates upon finishes
    pthread_detach(pthread_self());

    // main loop: listen and store data from clients
    while(1){
        // reset the buff
        memset(buff, BUFF_SIZE, 0);
        //printf("Hello?\n");
        // receive data from client
        rcvBytes = recvfrom(udp_server_socket, buff, BUFF_SIZE, 0, (struct sockaddr *) &clientAddr, &len);
        if(rcvBytes < 0){
            perror("Error: ");
            return 0;
        }
        //else printf("Received something\n");
        buff[rcvBytes] = '\0';
        for (int i = 0;i<rcvBytes;i++) printf("%d",buff[i]);
        printf("\n");
        printf("received: %d\n",rcvBytes);
        // get player_id of this data
        int player_id = buff[0];
        
        // find the corresponding player in the linkedlist
        Player *player = findPlayerInRoomById(players, player_id);

        // update UDP address of this client
        player->cliaddr = clientAddr;

        // copy the data received corresponds to this player
        memcpy(player->input_buffer, buff,7);
        player->bytes_received = rcvBytes;
    }

    
    pthread_exit(NULL);
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
    int gameStart =0;
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
    pthread_t tid;
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
    printf(YELLOW "Gameroom Server [%d] listening on %s with socket descriptor = %d\n" RESET, room_id, CHAR_TCP_PORT, listener);

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
                            printf(RED "(In waiting room %d), Client with socket descriptor %d got some error\n", room_id, i);
                            // perror(RED "Error in waiting room, receive data from connected clients" RESET);
                            continue;
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
                        
                        if (!gameStart) 
                        {
                            handleConnectedClients(i, buff);
                            gameStart = check_gameStart_condition(players, countPlayerInRoom(players));
                        }
                    }
                } // END handle data from client
                
                //send response 8 to all players in the room
                Player *p = players;
                Player *head = players;
                while (p != NULL) {
                    int fd = p->socket_descriptor;
                    sendResponse8(fd, head);
                    p = p->next;
                }

            } // END got new incoming connections
        } // END looping through file descriptors
        if (gameStart)
            {
                printf("Game is starting...\n");
                break;
            }

        // print currently active clients in game room
        printf(CYAN "\t(In waiting room %d) Currently active clients information:\n" RESET, room_id);
        Player *p = players;

        // printf("1\n");

        // if(p == NULL) printf("p is null\n");
        while(p != NULL){
            // printf("2\n");

            char ip[INET_ADDRSTRLEN];  // Buffer to store the IP address as a string
            inet_ntop(AF_INET, &(p->cliaddr.sin_addr), ip, INET_ADDRSTRLEN);  // Convert to string
            
            printf("\t\tClient with id = %d, socket = %d, address = [%s:%d], ready = %d\n", p->id, p->socket_descriptor, ip, ntohs(p->cliaddr.sin_port), p->ready);

            // printf("3\n");
            p = p->next;
        }
        
    } // END for(;;)

    
   // ------------------------SET UP UDP PORT -----------------------------
    int UDP_server_socket ;
    char buffer[BUFF_SIZE];
    struct sockaddr_in server_addr, client_addr;
    socklen_t udp_addr_len;
    // Create UDP socket, maximum attempt to recreate is 5 times, if fails more then delete the room -> will implement this later on
    if ((UDP_server_socket =  socket(AF_INET, SOCK_DGRAM,0))<0)
    {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // configure server address structure
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family =  AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port =  htons(UDP_SERV_PORT);

    // Bind the socket to the server address
    if (bind(UDP_server_socket, (const struct sockaddr *)&server_addr, sizeof(server_addr))<0)
    {
        perror("Bind failed");
        close(UDP_server_socket);
        exit(EXIT_FAILURE);
    }

    printf(YELLOW "UDP Server is running on port %d\n" RESET, UDP_SERV_PORT);

    while (1)
    {
        int sent_udp_port = sent_UDP_port(players, UDP_SERV_PORT);
        if (sent_udp_port) break;
        else continue;
    }
    //------------------ Initialize the game first ------------------
    Game *game =  (Game*) malloc(sizeof(Game));
    game->map = 4;
    game->tilemap =  loadTilemap(16, game->map);
    if (game->tilemap==NULL) 
    {
        printf("Error: map is not properly loaded\n");
        return 1;
    }
    game->game_loop=0;
    game->game_mode=0; //game mode 0  for deathmatch, game mode 1 for capture the flag
    game->gravity=5;
    game->items = NULL;
    game->players =  players;
    if (game->game_mode==1){
        game->Score_team1 = 0;
        game->Score_team2 = 0;
        }
    game->traps= NULL;
    total_players =  countPlayerInRoom(players);
    characterSpawner(game->players);
    // set up a number of buffer for each client
    
    char send_buffer[1024];
    int byteSerialized;
    int fresh_start =1;

    //----------------------------------Main Game--------------------------------------//
    // create a thread to handle listening from client (and broadcasting) data 
    if(pthread_create(&tid, NULL, listenFromClients, &UDP_server_socket) != 0){
        perror(RED "Error creating thread ");
    }
    struct timespec last_tick, current_time;
    clock_gettime(CLOCK_MONOTONIC, &last_tick);

    while (true)
        {
            //printf("inside the loop");
        clock_gettime(CLOCK_MONOTONIC, &current_time);
        long elapsed_ms  =  (current_time.tv_sec - last_tick.tv_sec)*1000 
                            + (current_time.tv_nsec - last_tick.tv_nsec)/1000000;
        // Listen from each client
        if (elapsed_ms >TICK_RATE)
        {
            int client_responded =0;
            // if no client has responded do not move the game state forward
            // Parsing message from player input and update player state
            Player *each;
            int id ;
            int movementx ;
            int movementy ;
            int action ;
            int interaction ;
            int sizex ;
            int sizey ;
            for (each =game->players;each!=NULL;each=each->next)
            {
                if (each->bytes_received!=7)  continue; //payload is always 5 bytes for each player
                else 
                {
                    id = each->input_buffer[0];
                    movementx = each->input_buffer[1];
                    movementy =  each->input_buffer[2];
                    action =  each->input_buffer[3];
                    interaction =  each->input_buffer[4];
                    sizex =  each->input_buffer[5];
                    sizey =  each->input_buffer[6];
                    printf("input:");
                    for (int i = 0;i<7;i++) printf("%d",each->input_buffer[i]);
                    printf("\n");
                    Player* player =  findPlayerInRoomById(game->players, id);
                    if (player ==NULL) 
                    {
                        printf("Player ID %d not found\n",id);
                        continue;                            
                    }
                    else
                    {
                        player->sizex=sizex;
                        player->sizey=sizey;
                        player->movement_x=movementx;
                        if (movementx==1) player->isFacingLeft = false;
                        if (movementx==2) player->isFacingLeft= true;
                        player->movement_y=movementy;
                        handleStateChange(player, action); // handle change in animation first
                        //handleInteraction(player, interaction);
                    }
                    each->bytes_received=0;
                    if (!client_responded) client_responded=1;

                }            
            }
            if (!client_responded) continue;
            // update and serialize players' info
            memset(send_buffer, 0, BUFF_SIZE);
            // check contact of walls and grounds
            Player *temp;
            for (temp=game->players;temp!=NULL;temp= temp->next)
            {
               check_player_contact(temp,game->tilemap);
            }
            // update player
            byteSerialized=0;
            for (temp=game->players;temp!=NULL;temp= temp->next)
            {
                update_player(temp,game);
                byteSerialized = serialize_player_info(send_buffer, byteSerialized, temp);
            }
            int byteSent = 0;
            for (temp=game->players;temp!=NULL;temp= temp->next)
            {
                addrlen =  sizeof(temp->cliaddr);
                byteSent =sendto(UDP_server_socket, send_buffer, byteSerialized,0, (const struct sockaddr *)&temp->cliaddr,addrlen);
                if (byteSent<=0)
                    {
                        char message[100];
                        sprintf(message, "Send failed for player id %d\n",temp->id);
                        perror(message);
                    }  
                else
                {
                    //
                }
                    
            }
            last_tick =  current_time;

        }
        else 
        {
            usleep(1000);
        }  
        }   

    return 0;
}
