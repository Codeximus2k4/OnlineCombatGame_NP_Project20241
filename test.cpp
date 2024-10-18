#include<stdio.h>
#include<sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <arpa/inet.h>
#include<ctype.h>
#include <stdlib.h>

struct Player {
    int id; // id of player
    char name[50]; // player's name
    int position_x; // player x position
    int position_y; // player y position
    int flip; // player looking right or left
    char action[50]; // player's current action
    int frame; // player's frame
    sockaddr_in cliaddr; // IPv4 address corresponding to each player
    Player *next; // next player in the list
};

Player *players = NULL;

// make a new Player based on the information provided
// input: all player's information
// output: pointer to a new Player
// dependencies: none
Player *makePlayer(int id, char name[], int position_x, int position_y, int flip, char action[], int frame, sockaddr_in cliaddr){
    Player *p = (Player*) malloc(sizeof(Player));
    p->id = id;
    strcpy(p->name, name);
    p->position_x = position_x;
    p->position_y = position_y;
    p->flip = flip;
    strcpy(p->action, action);
    p->frame = frame;
    p->cliaddr = cliaddr;

    p->next = NULL;

    return p;
}

// function to serialize Player info into a string
// input: pointer to struct Player
// output: string contains player info
// dependencies: none
char *serializePlayerInfo(Player *player){
    char string[10000]; // actual string that stores serialized player data
    char *result; // pointer to string
    char strnum[50]; // used for converting integer to array of char

    result = string;
    
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
    strcat(string, "|");

    // append player flip
    snprintf(strnum, sizeof(strnum), "%d", player->frame);
    strcat(string, strnum);

    return result;
}

// function to make Player from serialized string information
// input: serialized Player data format, sockaddr_in client address
// output: a pointer to a new player
Player *unserializePlayerInfo(char information[], sockaddr_in cliaddr){
    int id; 
    char name[50];
    int position_x;
    int position_y;
    int flip;
    char action[50];
    int frame;

    char *token;
    // get id
    token = strtok(information, "|");
    id = atof(token);

    // get name
    token = strtok(NULL, "|");
    strcpy(name, token);

    // get pos x
    token = strtok(NULL, "|");
    position_x = atof(token);

    // get pos y
    token = strtok(NULL, "|");
    position_y = atof(token);

    // get flip
    token = strtok(NULL, "|");
    flip = atof(token);

    // get action
    token = strtok(NULL, "|");
    strcpy(action, token);
    
    // get frame
    token = strtok(NULL, "|");
    frame = atof(token);

    Player *p = makePlayer(id, name, position_x, position_y, flip, action, frame, cliaddr);

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

// function to check if 2 sockaddr_in is the same
// input: sockaddr_in of client1 and client2
// output: true if 2 clients have same address:port
//         false otherwise
// dependencies: none;
bool checkSameAddress(sockaddr_in cli1, sockaddr_in cli2){
    // check same IP
    if(cli1.sin_addr.s_addr == cli2.sin_addr.s_addr){
        // if also same port
        if(cli1.sin_port == cli2.sin_port) return false;
    }

    return true;
}

// check if a client address is already in the list of stored players
// input: a client address
// output: true if client is in list
//         false otherwise
// dependencies: checkSameAddress()
bool clientInList(sockaddr_in *client){
    Player *p = players;

    while(p != NULL){
        if(checkSameAddress(p->cliaddr, *client) == true){
            return true;
        }
        p = p->next;
    }

    return false;
}

int main() {
    char *playerinfo;
    char information[100];
    char ip[50];
    sockaddr_in cliaddr1, cliaddr2;
    char ip_con[100];

    strcpy(ip, "1.1.1.1");

    cliaddr1.sin_family = AF_INET;
    cliaddr1.sin_port = 5000;
    inet_pton(AF_INET, ip, &cliaddr1.sin_addr);

    cliaddr1.sin_family = AF_INET;
    cliaddr1.sin_port = 5000;
    inet_pton(AF_INET, ip, &cliaddr2.sin_addr);

    strcpy(information, "1|hieu|5|10|1|attack|50");

    Player *player = unserializePlayerInfo(information, cliaddr1);

    if(!clientInList(&cliaddr1)){
        players = addPlayer(player);
        printf("different address, adding\n");
    }

    strcpy(information, "2|hoang|10|20|0|idle|10");

    player = unserializePlayerInfo(information, cliaddr2);

    if(!clientInList(&cliaddr2)){
        players = addPlayer(player);
        printf("different address, adding\n");
    }

    Player *p = players;

    while(p != NULL){
        playerinfo = serializePlayerInfo(p);

        printf("%s\n", playerinfo);

        p = p->next;
    }
   

    return 0;
}