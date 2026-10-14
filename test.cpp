#include<stdio.h>
#include<sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <arpa/inet.h>
#include<ctype.h>
#include <stdlib.h>
#include<unistd.h>

#define BUFF_SIZE 1024 // MAX UDP packet size is 1500 bytes

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
    char name[50]; // player's name
    int position_x; // player x position
    int position_y; // player y position
    int flip; // player looking right or left
    char action[50]; // player's current action
    sockaddr_in cliaddr; // IPv4 address corresponding to each player
    Player *next; // next player in the list
};

/*---------------------------------------------
Defining global variables
-----------------------------------------------
*/

// define list of players
Player *players = NULL; // pointer to head of linked list of players
int maxPlayer = 0; // keep count of total players in the room

/*
--------------------------------------------------
UNDER DEVELOPMENT VARIABLES - do not delete
--------------------------------------------------
// used for IPC with fork() and pipe()
int p_to_c[20][2]; // pipe to write from parent -> children processes
int c_to_p[20][2]; // pipe hanle writing from children -> parent
*/

/*---------------------------------------------
Defining functions
-----------------------------------------------
*/

// - make a new Player based on the information provided
// - input: all player's information
// - output: pointer to a new Player
// - dependencies: none
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

// - function to make Player from serialized string information
// - input: serialized Player data format, sockaddr_in client address
// - output: a pointer to a new player with information filled
// - dependencies: makePlayer()
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


int main (int argc, char *argv[]) {
    char information[100];
    char *infor;
    sockaddr_in cliaddr;
    char ip[50] = "1.1.1.1";

    strcpy(information, "1|hieu|5|10|1|attack");
    inet_pton(AF_INET, ip, &cliaddr.sin_addr);

    Player *player = unserializePlayerInfo(information, cliaddr);

    infor = serializePlayerInfo(player);
    printf("%s\n", infor);
    printf("%d", strlen(infor));


    return 0;
}
