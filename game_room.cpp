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
char input_buffer[5][BUFF_SIZE]; // hold data received from calling recvfrom() UDP

/*---------------------------------------------
Defining functions
-----------------------------------------------
*/

//function to check the condition to start the game (as of now it is that all players including the host are ready)
// -  input: head pointer of Players linked list and the total number of player
// -  output: 1 for game ready to start , 0 for not ready
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
    data[0] = '6'; // first byte is request type
    data[1] = status + '0'; // second byte is status

    // send to client (2 bytes)
    if( (sendBytes = send(connectfd, data, 2, 0)) < 0){
        perror("Error");
    };
    
    // print to check
    printf(YELLOW "Bytes sent to client=%d, type=%c, status=%c\n" RESET, sendBytes, data[0], data[1]);
}

// function to handle data when connected clients want to send something to server (for e.g: player ready state)
// - handle request type 7 from clients
struct Message_holder
{
    int clientfd;
    char message[6];
    int length;   
    Message_holder *next; 
};
Message_holder *message =  NULL,*tail = NULL; 
Message_holder* find_holder(int clientfd, Message_holder *start)
{
    if (start==NULL) return NULL;
    else
    {
        while (start!=NULL)
        {
            if (start->clientfd==clientfd) return start;
            else start = start->next;
        }
    }
    return start;
}
void handleConnectedClients(int clientfd, char buff[BUFF_SIZE + 1]) {
    Message_holder *temp =  find_holder(clientfd, message);
    if (temp==NULL)
    {
        Message_holder *new_holder =  (Message_holder*) malloc(sizeof(Message_holder));
        new_holder->clientfd= clientfd;
        new_holder->next =NULL;
        new_holder->length = 0;
        if (message==NULL) 
        {
            message= new_holder;
            tail =  new_holder;
        }
        else 
        {
            tail->next =  new_holder;
            tail  = new_holder;
        }
        temp = new_holder;
    }
    int sendBytes;
    int recvBytes;
    //printf("OK\n");
    if( (recvBytes = recv(clientfd, buff, 2, 0)) < 0){
        perror("Error");
    } else if(recvBytes == 0){
        fprintf(stdout, "Client closes connection\n");
        return;
    }
    //printf("%d\n",recvBytes);
    for (int i=0;i<recvBytes;i++)
    {
        temp->message[temp->length+i]= buff[i];
        temp->length+=1;
    }
    //printf("length: %d\n",temp->length);
    if (temp->length==2)
    {
        int player_id =  temp->message[0];
        //printf("id: %d\n",player_id);
        int ready =  temp->message[1];
        Player *correspondingPlayer = findPlayerInRoomById(players, player_id);
        // if (correspondingPlayer ==NULL) printf("Not found player\n");
        // else printf("Found player ID: %d\n", correspondingPlayer->id);
        correspondingPlayer->ready = ready;
        printf(GREEN "Player with id %d on has ready status : %d\n"RESET,correspondingPlayer->id, ready);
        correspondingPlayer->udp_port_byte_sent = 0;
        temp->length = 0;
    }
    //printf("Message is incomplete\n");
    //printf("Received %d status from players\n",ready);
    // update internal linked list

}

// - function : sending the game UDP port to all clients
// - input    : linked list of players
// - output   : 1 if finished sending, 0 if not finished or error occured
int sent_UDP_port(Player *head, int udp_port)
{
    // implementing non-blocking send using select
    fd_set writefds;
    struct timeval timeout; //timeout for each client is 1 second
    timeout.tv_sec = 1;
    timeout.tv_usec= 0;
    char message[3];
    message[0]=7;
    message[1]= udp_port/256;
    message[2]= udp_port%256; 
    int finished_sending = 1;
    // first we check the writability of each socket first
    for (Player *p=head; p!=NULL;p=p->next)
    {
        int total_message_length =3; // we have to send 3 bytes to each client
        int port_bytes_sent =  p->udp_port_byte_sent;
        int tcp_client_sockfd =  p->socket_descriptor;
        if (port_bytes_sent==total_message_length) continue;
        else
        {
            finished_sending=0;
            FD_ZERO(&writefds);
            FD_SET(tcp_client_sockfd, &writefds);
            
            int activity =  select(tcp_client_sockfd+1, NULL, &writefds, NULL, &timeout);
            if (activity >0)
            {
                int bytes_sent = send(tcp_client_sockfd, message+port_bytes_sent,3- port_bytes_sent, 0);
                if (bytes_sent<0)
                {
                    char err_message[80];
                    sprintf(err_message,"Error with Room message type 7 at client %d:",p->id);
                    perror(err_message);
                }
                else p->udp_port_byte_sent+=bytes_sent;
            }
            else if (activity==0)
            {
                printf(YELLOW "Server: timeout occur when sending udp port to clients\n"RESET);                
            }
            else 
            {
                perror("Server: select failed while sending udp port");                
            }
        }
    }
    return finished_sending;

}

// - function to handle listening data from client using thread
// - input: udp_server_socket that is used for communicating with clients
// 
void *listenFromClients(void *arg){
    // get udp_server_socket
    int udp_server_socket = *(int *) arg;

    // init other data
    char buff[BUFF_SIZE + 1];
    int rcvBytes;
    sockaddr_in clientAddr;
    int len = sizeof(clientAddr);


    // detach this thread so that it terminates upon finishes
    pthread_detach(pthread_self());

    // main loop: listen and store data from clients
    while(1){
        // reset the buff
        memset(buff, BUFF_SIZE, 0);

        // receive data from client
        rcvBytes = recvfrom(udp_server_socket, buff, BUFF_SIZE, 0, (struct sockaddr *) &clientAddr, &len);
        if(rcvBytes < 0){
            perror("Error: ");
            return 0;
        }
        buff[rcvBytes] = '\0';

        // get player_id of this data
        int player_id = buff[0];
        
        // find the corresponding player in the linkedlist
        Player *player = findPlayerInRoomById(players, player_id);

        // update UDP address of this client
        player->cliaddr = clientAddr;

        // copy the data received corresponds to this player
        strcpy(player->input_buffer, buff);
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
    int gameStart =0; // has the game started yet?
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

    // convert TCP port to string
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
    //printf("OK\n");
    struct timeval tv;
    tv.tv_sec = 1;
    tv.tv_usec = 0;
    // main loop
    while(1){
        // int choice =0;
        // scanf("%d",choice);
        // if (choice ==0)
        // {
        //     close(UDP_server_socket);
        //     close(listener);
        //     return 0;
        // }
        //fprintf(stdout,"Game room is running\n");
        read_fds = master; // copy master set so we don't modify it, we need original version of this set

        // note that select() hanldes from file descriptor 0 -> fdmax (if arguments are provided as below)
        // however, select() will only handle those that have been added to `read_fds`
        if(select(fdmax + 1, &read_fds, NULL, NULL, &tv) == -1){
            perror("select");
            exit(4);
        }
        // run through the existing connections looking for data to read
        for(int i = 0; i <= fdmax; i++){
            total_players = countPlayerInRoom(players);
            memset(buff, 0, sizeof(buff));

            // if we have one (can be new connection or data from connected clients)
            if(FD_ISSET(i, &read_fds)){
                //printf("socket %d has sth to talk\n", i);

                if(i == listener){
                    // handle new connections
                    addrlen = sizeof(remoteaddr);
                    newfd = accept(listener, (struct sockaddr*) &remoteaddr, &addrlen);

                    if(newfd == -1){
                        perror("accept");
                    } 
                    else {
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
                                perror("recv");
                            }
                        } else { // check if we can accept this new connection or not 
                            int can_join = 1;

                            // get user_id
                            //printf("ckpt1\n");
                            if( (nbytes = recv(newfd, buff, 1, 0)) < 0){
                                perror("Error");
                            } else if(nbytes == 0){
                                fprintf(stdout, "Client closes connection\n");
                                //return;
                                continue;
                            }
                            //printf("ckpt2\n");
                            int player_id = buff[0];

                            // check total players

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
                                    perror("msgsnd failed");
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
                } 
                
                else 
                
                {
                    // handle data from connected clients (players who have already successfully connected to game room)
                    if( (nbytes = recv(i, buff, 1, 0)) <= 0){ // get one byte only (which is opcode, so we can process)
                        // got error or connection closed by client (kick this client out of the game room)

                        if(nbytes == 0){
                            // connection closed
                            printf("gameroom server: socket %d hung up\n", i);
                        } else {
                            perror("recv");
                        }

                        printf("socket %d , for real?\n",i);
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
                            perror("msgsnd failed");
                            exit(1);
                        }
                        printf("Child: Message sent to parent. %s\n", message.text);

                    } 
                    else 
                    {
                        // if we got some data from connected clients (i.e. handling ready state of players), process
                        printf("OK\n");
                        //waiting room - main game handle
                        handleConnectedClients(i,buff);
                        if (!gameStart) gameStart = check_gameStart_condition(players,total_players);
                    }
                    }
                
                }
                }// END handle data from client 
           // printf("Has the game started? %d\n",gameStart);
            if (!gameStart) continue; // if the starting condition is not satisfied continue to wait
            else 
                {
                        int finished_sent_UDP = sent_UDP_port(players,UDP_SERV_PORT); // if all the clients have been sent the UDP port, then we start!!
                        if (!finished_sent_UDP) continue;
                        else 
                        {
                        break;    
                

            } // END got new incoming connections
        } // END looping through file descriptors
    } //END of first connection loop

    // ---------------------- END OF WAITING PERIOD ------------------------

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


    //------------------ Initialize the game first ------------------
    Game *game =  (Game*) malloc(sizeof(Game));
    game->game_loop=0;
    game->game_mode=0; //game mode 0  for deathmatch, game mode 1 for capture the flag
    game->gravity=-1;
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
    
    int byteReceived[total_players];
    char send_buffer[1024];
    int byteSerialized;
    int fresh_start =1;

    //----------------------------------Main Game--------------------------------------//

    // create a thread to handle listening from client (and broadcasting) data 
    if(pthread_create(&tid, NULL, listenFromClients, &UDP_server_socket) != 0){
        perror(RED "Error creating thread ");
    }

    while (true)
                {
            // printf("Game Running\n");

        // Listen from each client
        int client_responded =0;
        
        

        if (!client_responded)
        {
                // if no client has responded do not move the game state forward
            //printf("No client has responded yet\n");
            continue;
        }
        //printf("Someone said something !!!\n");
        fresh_start=0;
        // Parsing message from player input and update player state
        for (int each =0;each<total_players;each++)
        {
            if (byteReceived[each]!=5)  continue; //payload is always 5 bytes for each player
            else 
            {
                int id =  input_buffer[each][0];
                int movementx = input_buffer[each][1];
                int movementy =  input_buffer[each][2];
                int action =  input_buffer[each][3];
                int interaction =  input_buffer[each][4];

                Player* player =  findPlayerInRoomById(game->players, id);
                if (player ==NULL) 
                {
                    printf("Player ID %d not found\n",id);
                    continue;                            
                }
                else
                {
                    player->movement_x=movementx;
                    handleActionChangeFromInput(player, action); // handle change in animation first
                    //if (movementy!=0) handleJumpCrouch(player, movementy);
                    //handleInteraction(player, interaction);
                }

            }            
        }

        // update and serialize players' info
        memset(send_buffer, 0, BUFF_SIZE);
        byteSerialized=0;
        Player *temp;
        for (temp=game->players;temp!=NULL;temp= temp->next)
        {
            update_player(temp);
            byteSerialized = serialize_player_info(send_buffer, byteSerialized, temp);
        }
        printf("Payload size is %d bytes\n",byteSerialized);
        //Let's send to each
        int byteSent = 0;
        for (temp=game->players;temp!=NULL;temp= temp->next)
        {
            addrlen =  sizeof(temp->cliaddr);
            byteSent =sendto(UDP_server_socket, send_buffer, byteSerialized,0, (const struct sockaddr *)&temp->cliaddr,temp->addrlen);
            if (byteSent<=0)
                {
                    char message[100];
                    sprintf(message, "Send failed for player id %d",temp->id);
                    perror(message);
                }  
            else 
            {
                printf("well, obviously server sent %d bytes ?\n",byteSent);
            }
                
        }

        }     

 return 0;
}    
    // ------------------------ INGAME PERIOD -----------------------------
    // not implemented

