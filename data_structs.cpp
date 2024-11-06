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

// int id; // id of player
// sockaddr_in cliaddr; // IPv4 address corresponding to each player
// Player *next; // next player in the list
struct Player {
    int id; // id of player
    sockaddr_in cliaddr; // IPv4 address corresponding to each player
    Player *next; // next player in the list
};

// int id;
// int tcp_port; // TCP port assign for each game room to handle client connection
// int udp_port; // UDP port assign for each game room to transfer data
// Player *players; // store the head pointer to the list of Players in this room
// Room *next;
struct Room {
    int id;
    int tcp_port; // TCP port assign for each game room to handle client connection
    int udp_port; // UDP port assign for each game room to transfer data

    Player *players; // store the head pointer to the list of Players in this room
    Room *next;
};

// - add a player to a room
// - input: head pointer to list of rooms, id of room to add, pointer to a Player
// - output: head of list of rooms, NULL if operation unsuccessful
// - dependencies: none
Room *addPlayerToRoom(Room *rooms, int room_id, Player *player){
    // find room to add
    Room *room = rooms;
    int found = 0;

    while(room != NULL){
        if(room->id == room_id){
            break; 
            found = 1;
        }
        room = room->next;
    }

    if(found == 0) return NULL;

    // if there are no players yet
    if(room->players == NULL){
        room->players = player;

        return room;
    }

    // else add to the end of the linked list
    Player *p = room->players;
    while(p->next != NULL){
        p = p->next;
    }

    p->next = player;

    return rooms;
}

// - add a player to list of logged in players
// - input: head pointer to list of logged in players, pointer to a Player
// - output: head of list of logged in players
// - dependencies: none
Player *addPlayerToLoginList(Player *head, Player *player){
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

    p->players = NULL;

    p->next = NULL;

    return p;
}

// - function to count total number of players in a room
// - input: a pointer to a room
// - output: number of players in the room
// - dependencies: none
int countPlayerInRoom(Room *room){
    Player *p = room->players;
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
        int totalPlayerInRoom = countPlayerInRoom(p);

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

