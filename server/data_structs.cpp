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

/*---------------------------------------------
Defining Player structs
-----------------------------------------------
*/

//first node --> hit box of the player
//from second node --> hit box of chieu thuc
struct hitBox {
    int x, y; //coordinate of the top left position
    int width, height;
    int buff; //an vat pham --> chieu dc buff
    hitBox *next;
};

// the game room (sub process) manages the list of player in the room
// int id; // id of player
// sockaddr_in cliaddr; // IPv4 address corresponding to each player
// Player *next; // next player in the list
struct Player {
    int id; // id of player
    float x, y; //position of player
    int hp; //health point
    int hit; //= 0 --> isnt' hit, = 1 --> is hit
    float speed[2]; //speed_x, speed_y
    int dmg; //sat thuong minh gay ra
    int jump_cool_down;
    int atk_cool_down;
    int ready;
    sockaddr_in cliaddr; // IPv4 address corresponding to each player
    Player *next; // next player in the list
    hitBox *selfHitBox; //
};



//linked list of items
//each item is at different pos on the screen
struct items {
    int x, y; //position
    int type; //type = 0 --> buff hp, type = 1 --> buff dame chieu
    int buff_amount;
    items *next;
};

//there are trap doors on the map. when player hits it, a projectile is fired
//speed and direction of the projectile
struct projectile {
    float speed[2]; 
    int dmg;
    projectile *next;
};

//
struct tile {
    int x, y; //position
    int grid_tile ; //0: off_grid tile, 1: grid_tile
    tile *next;
};

// struct npc {

// }

struct vfx {
    int x, y;
    int type;
    int duration; //theo vong lap
    vfx *next;
};


// the room list is managed in main server, while list of players in a room is managed in sub process
// int id;
// int tcp_port; // TCP port assign for each game room to handle client connection
// int udp_port; // UDP port assign for each game room to transfer data
// Room *next;
struct Room {
    int id;
    int tcp_port; // TCP port assign for each game room to handle client connection
    int udp_port; // UDP port assign for each game room to transfer data
    int started; 
    int total_players;
    Room *next;
};

// message format
// message send from sub processes (game rooms) to main server
// type = 1 (indicate that server receives)
// text = [room_id][game_start_yet?][num_players_in_room]
// game_start_yet? = '0' --> waiting, = '1' --> started, = '2' --> finish
struct ipc_msg {
    long type;
    char text[4];
};

// - add a player to list of players in the room
// - input: head pointer to list of players in the room, pointer to a Player
// - output: head of list of players
// - dependencies: none
Player *addPlayerToPlayersInRoom(Player *head, Player *player){
    // if there are no players yet
    if(head == NULL){
        head = player;
        return head;
    }

    // else add to the end of the linked list
    Player *p = head;
    while(p->next != NULL){
        p = p->next;
    }

    p->next = player;

    return head;
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

// - add a room to list of rooms
// - input: head pointer to list of rooms, pointer to a room
// - output: update head of linked list
// - dependencies: none
Room *addRoom(Room *head, Room *room) {
    // if there are no rooms yet
    if(head == NULL){
        head = room;

        return head;
    }

    // else add to the end of the linked list
    Room *p = head;
    while(p->next != NULL){
        p = p->next;
    }

    p->next = room;

    return head;
}

// - make a new Room based on the information provided
// - input: room's id, tcp_port, udp_port, players pointer of Room will be initilized as NULL, call addPlayer() to add player to a room
// - output: pointer to a new Room
// - dependencies: none
Room *makeRoom(int id, int tcp_port, int udp_port){
    Room *p = (Room *) malloc(sizeof(Room));
    p->id = id;
    p->tcp_port = udp_port;
    p->udp_port = udp_port;
    p->next = NULL;

    return p;
}

// - function to count total number of players in a room
// - input: the head of the player list in the room
// - output: number of players in the room
// - dependencies: none
int countPlayerInRoom(Player *head){
    Player *p = head;
    int totalPlayerCount = 0;

    if(p == NULL) return 0;

    while(p != NULL){
        totalPlayerCount++;
        p = p->next;
    }

    return totalPlayerCount;
}

// - function to count total number of rooms in server
// - input:  pointer to head of list of rooms
// - output: number of rooms in the list
// - dependencies: none
int countRooms(Room *head) {
    Room *p = head;
    int totalRoomCount = 0;

    if(p == NULL) return 0;

    while(p != NULL){
        totalRoomCount++;
        p = p->next;
    }

    return totalRoomCount;
}

// - function to serialize information of list of rooms available
// - input: char string to store result, head pointer to list of rooms
// - output: none
// - dependencies: countPlayerInRoom(), countRooms()
// - note: returned string will have format: [number_of_rooms][id_room1][number_players_room1][id_room2][number_players_room2][...]
// if no rooms are available then return result = "0"
void serializeRoomInformation(char result[], Room *head){
    int totalRooms = 0;
    char strnum[50]; // used for converting integer to array of char

    // if list of rooms is empty
    if(head == NULL) {
        strcpy(result, "0");
        // strcpy automatically sets last element to '\0'

        return;
    }

    // count total of available rooms and set it to first byte
    int totalRoom = countRooms(head);
    snprintf(strnum, sizeof(strnum), "%d", totalRoom);
    strcpy(result, strnum);

    Room *p = head;
    while(p != NULL){
        // set first byte to room_id
        snprintf(strnum, sizeof(strnum), "%d", p->id);
        strcat(result, strnum);

        // count players in room
        int totalPlayerInRoom = p->total_players;

        // printf("Total players in room %d is: %d\n", p->id, totalPlayerInRoom);

        // set second byte to # of players
        snprintf(strnum, sizeof(strnum), "%d", totalPlayerInRoom);
        strcat(result, strnum);

        // increment
        totalRooms++;
        p = p->next;
    }

    // printf("totalRooms=%d\n", totalRooms);
    // printf("result=%s\n", result);
}

//message format: [room_id][num_players_in_room][started]
//input: the message, the room_id, the head of the player list, started
void serializeIpcMsg(ipc_msg *message, int room_id, Player *head, int started) {
    message->type = 1;
    message->text[0] = room_id + '0';
    message->text[1] = countPlayerInRoom(head) + '0';
    message->text[2] = started + '0';
    message->text[3] = '\0';
}

// - function to find room by id
// - input: head pointer to list of rooms, room_id
// - output: pointer to the room, NULL if doesn't exist room with the id
// - dependencies: none
Room *findRoomById(Room *head, int room_id){
    if(head == NULL) return NULL;
    
    Room *p = head;
    while(p != NULL){
        if(p->id == room_id) {
            break;
        }
    }

    return p;
}

