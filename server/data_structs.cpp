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
    int socket_descriptor; // socket descriptor corresponding for each player (to handle select())

    float x, y; // position of player
    int hp; // health point
    int hit; // = 0 --> isnt' hit, = 1 --> is hit
    float speed[2]; // speed_x, speed_y
    int dmg; // sat thuong minh gay ra
    int jump_cool_down;
    int atk_cool_down;
    int ready;
    sockaddr_in cliaddr; // IPv4 address corresponding to each player
    hitBox *selfHitBox; //

    Player *next; // next player in the list
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
    int ready;
    int total_players;
    Room *next;
};

// message format
// message send from sub processes (game rooms) to main server
// type = 1 (indicate that server receives)
// text = [room_id][num_players_in_room][started]
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
// - input: id, socket descriptor, IPv4 address of player, all other values will be set to 0 or NULL
// - output: pointer to a new Player
// - dependencies: none
Player *makePlayer(int id, int socket_descriptor, sockaddr_in cliaddr){
    Player *p = (Player*) malloc(sizeof(Player));
    p->id = id;
    p->socket_descriptor = socket_descriptor;
    p->cliaddr = cliaddr;
    p->x = 0;
    p->y = 0;
    p->hp = 0;
    p->hit = 0;
    memset(p->speed, 0, sizeof(p->speed));
    p->dmg = 0;
    p->jump_cool_down = 0;
    p->atk_cool_down = 0;
    p->ready = 0;
    p->selfHitBox = NULL;

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
    p->tcp_port = tcp_port;
    p->udp_port = udp_port;
    p->started = 0; // when we create a room then this room must not be started yet
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

// - function to count total number of players that are ready in a room
// - input: the head of the player list in the room
// - output: number of players in the room
// - dependencies: none
int countPlayerReadyInRoom(Player *head) {
    Player *p = head;
    int totalPlayerReadyCount = 0;

    if(p == NULL) return 0;

    while(p != NULL){
        if (p->ready == 1) totalPlayerReadyCount++;
        p = p->next;
    }

    return totalPlayerReadyCount;
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
    int offset = 0; //offset of result[]

    // if list of rooms is empty
    if(head == NULL) {
        strcpy(result, "0");
        // strcpy automatically sets last element to '\0'

        return;
    }

    // count total of available rooms and set it to first byte
    int totalRoom = countRooms(head);
    offset += snprintf(strnum, sizeof(strnum), "%d", totalRoom);
    strcpy(result, strnum);

    Room *p = head;

    while(p != NULL){
        // set first byte to room_id
        offset += snprintf(strnum, sizeof(strnum), "%d", p->id);
        strcat(result, strnum);

        // count players in room
        int totalPlayerInRoom = p->total_players;

        // printf("Total players in room %d is: %d\n", p->id, totalPlayerInRoom);

        // set second byte to # of players
        offset += snprintf(strnum, sizeof(strnum), "%d", totalPlayerInRoom);
        strcat(result, strnum);

        int room_tcp_port = p->tcp_port;
        // convert TCP port of room network byte (2 bytes) because there are 65536 ports
        uint16_t byte_room_tcp_port = htons(room_tcp_port);
        // append these 2 bytes to response packet
        memcpy(result + offset, &byte_room_tcp_port, 2);

        // increment
        totalRooms++;
        p = p->next;
        offset += 2;
    }

    // printf("totalRooms=%d\n", totalRooms);
    // printf("result=%s\n", result);
}

// message format: [room_id][num_players_in_room][started]
// input: the message, the room_id, the head of the player list
void serializeIpcMsg(ipc_msg *message, int room_id, Player *head) {
    int ready = 0; //the ready state of the room
    message->type = 1;
    message->text[0] = room_id + '0';
    message->text[1] = countPlayerInRoom(head) + '0';
    int num_players_ready = countPlayerReadyInRoom(head);
    if (num_players_ready == 4) ready = 1; //full 4 players are ready
    else ready = 0;
    message->text[2] = ready + '0';
    message->text[3] = '\0';
}

//message format: [type = '8'][num_of_players_in_room][player_id1][ready_1][,,.]
//input: result string, head of the player list of the room
int serializePlayersInRoomInformation(char result[], Player *head) {
    result[0] = '8';
    result[1] = countPlayerInRoom(head);
    if (result[1] == 0) {
        result[2] = '\0';
        return 3;
    }
    Player *p = head;
    int offset = 2; 
    while (p != NULL) {
        result[offset++] = p->id;
        result[offset++] = p->ready;
        p = p->next;
    }
    result[offset] = '\0';
    return offset+1;
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
        p = p->next;
    }

    return p;
}

// - function to find player in list of players in the room by ID
// - input: head pointer to list of players in room, player ID
// - output: pointer to the corresponding player or NULL if not found
Player *findPlayerInRoomById(Player *head, int player_id){
    if(head == NULL) return NULL;

    Player *p = head;
    while(p != NULL){
        if(p->id == player_id){
            break;
        }
        p = p->next;
    }

    return p;
}

// - function to find player in list of players in the room by corresponding socket descriptor 
// - input: head pointer to list of players in room, socket descriptor
// - output: pointer to the corresponding player or NULL if not found
Player *findPlayerInRoomBySocketDescriptor(Player *head, int socket_descriptor){
    if(head == NULL) return NULL;

    Player *p = head;
    while(p != NULL){
        if(p->socket_descriptor == socket_descriptor){
            break;
        }
        p = p->next;
    }

    return p;
}

// - function to remove a player from the list of players in the room
// - input: head pointer to the list of players, pointer to the player to remove
// - output: updated head pointer of the list of players
Player *removePlayerFromListPlayers(Player *head, Player *removePlayer){
    // if list of players empty
    if(head == NULL){
        return NULL;
    }

    // if we need to remove head
    if(head == removePlayer){
        head = head->next;
        free(removePlayer);

        printf("List of players updated (player to remove was at the head of list)\n");
        return head;
    }

    // traverse to find previous node 
    Player *temp = head;
    while(temp != NULL && temp->next != removePlayer){
        temp = temp->next;
    }

    // if the player to remove is not found
    if(temp == NULL){
        printf("Player not found in the list, list unchanged\n");
        return head;
    }

    // remove the node
    temp->next = removePlayer->next;
    free(removePlayer);
    printf("List of players updated (player to remove was not at head of the list)\n");
    return head;
}

// - function to remove a room from the list of rooms 
// - input: head pointer to the list of rooms, pointer to the room to remove
// - output: updated head pointer of the list of rooms
Room *removeRoomFromListRooms(Room *head, Room *removeRoom){
    // if list of room empty
    if(head == NULL){
        return NULL;
    }

    // if we need to remove head
    if(head == removeRoom){
        head = head->next;
        free(removeRoom);

        printf("List of rooms updated (room to remove was at the head of list)\n");
        return head;
    }

    // traverse to find previous node 
    Room *temp = head;
    while(temp != NULL && temp->next != removeRoom){
        temp = temp->next;
    }

    // if the player to remove is not found
    if(temp == NULL){
        printf("Room not found in the list, list unchanged\n");
        return head;
    }

    // remove the node
    temp->next = removeRoom->next;
    free(removeRoom);
    printf("List of rooms updated (room to remove was not at head of the list)\n");
    return head;
}