#include<stdio.h>
#include<sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <arpa/inet.h>
#include<ctype.h>
#include <stdlib.h>
#include<unistd.h>

#define BUFF_SIZE 1024 // MAX UDP packet size is 1500 bytes


/*
Data format: data type is in string
id|name|position_x|position_y|flip|action

- id: id of player
- name: name of player
- position_x, position_y: coordinate of player
- flip: 1 (True) or 0 (False)
- action: action of player
- frame: frame number of player
*/
struct Player {
    int id; // id of player
    char name[50]; // player's name
    int position_x; // player x position
    int position_y; // player y position
    int flip; // player looking right or left
    char action[50]; // player's current action
    sockaddr_in cliaddr; // IPv4 address corresponding to each player
    Player *next; // next player in the list
};

// define variables
Player *players = NULL; // pointer to head of linked list of players
int maxPlayer = 0; // keep count of total players in the room

// make a new Player based on the information provided
// input: all player's information
// output: pointer to a new Player
// dependencies: none
Player *makePlayer(int id, char name[], int position_x, int position_y, int flip, char action[], sockaddr_in cliaddr){
   Player *p = (Player*) malloc(sizeof(Player));
    p->id = id;
    strcpy(p->name, name);
    p->position_x = position_x;
    p->position_y = position_y;
    p->flip = flip;
    strcpy(p->action, action);
    p->cliaddr = cliaddr;

    p->next = NULL;

    return p;
}

// add a player to a list of existing player, or create a new list if no players exist
// input: a Player
// output: update "players" pointer
// dependencies: none
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

// function to serialize Player info into a string
// input: pointer to struct Player
// output: pointer to string contains player information
// dependencies: none
char *serializePlayerInfo(Player *player){
    char *string = (char *) malloc(10000); // string pointer that points to string storing serialized player data
    char strnum[50]; // used for converting integer to array of char
    
    // append player id
    snprintf(string, sizeof(string), "%d", player->id);
    strcat(string, "|");

    // append player name
    strcat(string, player->name);
    strcat(string, "|");

    // append position x
    snprintf(strnum, sizeof(strnum), "%d", player->position_x);
    strcat(string, strnum);
    strcat(string, "|");

    // append position y
    snprintf(strnum, sizeof(strnum), "%d", player->position_y);
    strcat(string, strnum);
    strcat(string, "|");

    // append player flip
    snprintf(strnum, sizeof(strnum), "%d", player->flip);
    strcat(string, strnum);
    strcat(string, "|");

    // append player action 
    strcat(string, player->action);

    return string;
}

// function to make Player from serialized string information
// input: serialized Player data format, sockaddr_in client address
// output: a pointer to a new player with information filled
// dependencies: makePlayer()
Player *unserializePlayerInfo(char information[], sockaddr_in cliaddr){
    int id; 
    char name[50];
    int position_x;
    int position_y;
    int flip;
    char action[50];
    int frame;

    char string[500]; // copy string from information so that we dont mess with original information
    strcpy(string, information);


    char *token;
    // get id
    token = strtok(string, "|");
    id = atoi(token);

    // get name
    token = strtok(NULL, "|");
    strcpy(name, token);

    // get pos x
    token = strtok(NULL, "|");
    position_x = atoi(token);

    // get pos y
    token = strtok(NULL, "|");
    position_y = atoi(token);

    // get flip
    token = strtok(NULL, "|");
    flip = atoi(token);

    // get action
    token = strtok(NULL, "|");
    strcpy(action, token);

    Player *p = makePlayer(id, name, position_x, position_y, flip, action, cliaddr);

    return p;
}   

// function to publish player information to all other clients
// input: data from a single client that needs to be broadcasted
void broadCastData(Player players) {
    
}

// function to check if 2 sockaddr_in is the same
// input: sockaddr_in of client1 and client2
// output: true if 2 clients have same address:port
//         false otherwise
// dependencies: none
bool checkSameAddress(sockaddr_in cli1, sockaddr_in cli2){
    // check same IP
    if(cli1.sin_addr.s_addr == cli2.sin_addr.s_addr){
        // if also same port
        if(cli1.sin_port == cli2.sin_port) return true;
    }

    return false;
}

// check if a client address is already in the list of stored players
// input: a client address
// output: true if client is in list
//         false otherwise
// dependencies: checkSameAddress()
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

int main (int argc, char *argv[]) {
    int sockfd, rcvBytes, sendBytes;
    char buff[BUFF_SIZE + 1];
    struct sockaddr_in servaddr;
    struct sockaddr_in cliaddr; // list of clients addresses
    socklen_t addr_len = sizeof(struct sockaddr_in); // size of address structure, in this case size of IPv4 structure
    int SERV_PORT;
    char cli_addr[100];
    char *result; // result string to return to client
    char errorString[10000]; // actual string return to client (used in case of error)
    

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

    //Step 1: Construct socket
        if((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0){
        perror("Error constructing socket: ");
        return 0;
    }
    fprintf(stdout, "Successfully created socket\n");

    //Step 2: Bind address to socket
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET; // user IPv4
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY); // set server to accept connection from any network interface (IPv4 only)
    servaddr.sin_port = htons(SERV_PORT); // set port of the server

    if(bind(sockfd, (struct sockaddr *) &servaddr, sizeof(servaddr))){
        perror("Error binding socket: ");
        return 0;
    }
    printf("Server started. Listening on port: %d\n", SERV_PORT);


    //Step 3: Communicate with client
    while(1){
        // reset the buff
        memset(buff, BUFF_SIZE, 0);

        // receive new connection 
        rcvBytes = recvfrom(sockfd, buff, BUFF_SIZE, 0, (struct sockaddr *) &cliaddr, &addr_len);
        if(rcvBytes < 0){
            perror("Error receiving data from client: ");
            continue;
        }
        buff[rcvBytes] = '\0';

        // now the last character in buff is \n -> makes the string len + 1, 
        // we need to remove this character
        if(buff[strlen(buff) - 1] == '\n') buff[strlen(buff) - 1] = '\0';

        // if client address is not in list of players
        if(!clientInList(&cliaddr)){
            // store client address into cli_addr variable
            inet_ntop(AF_INET, (void *) &cliaddr.sin_addr, cli_addr, INET_ADDRSTRLEN);

            printf("A new connection arrived from [%s:%d], creating new player\n", cli_addr, ntohs(cliaddr.sin_port));

            // create a new Player, associated the Player with address
            Player *newPlayer = unserializePlayerInfo(buff, cliaddr);

            // update list of players
            maxPlayer++;
            players = addPlayer(newPlayer);

            // logs out information
            printf("A new player has been created with id assigned as %d, forking a new process for this client\n", newPlayer->id);

            // fork a new process to handle this client
            pid_t pid = fork();

            if(pid < 0){
                perror("Fork failed\n");
            } else if(pid == 0){

            }
        }
        
        // process data received
        result = buff;

        // print received client address, port and data received
        //printf("[%s:%d]: %s\n", cli_addr, ntohs(cliaddr.sin_port), buff);

        // send data to client
        sendBytes = sendto(sockfd, result, strlen(result), 0, (struct sockaddr *) &cliaddr, addr_len);
        if(sendBytes < 0){
            perror("Error sending data to client: ");
            return 0;
        }
    }

    return 0;
}
