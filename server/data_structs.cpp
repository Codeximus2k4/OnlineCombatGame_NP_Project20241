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



// the game room (sub process) manages the list of player in the room
// int id; // id of player
// sockaddr_in cliaddr; // IPv4 address corresponding to each player
// Player *next; // next player in the list
int min(int a , int b) {
    if (a < b) return a;
    else return b;
}

int max(int a, int b) {
    if (a > b) return a;
    else return b;
}

struct Hitbox {
    int offset_x;
    int offset_y;
    int width;
    int height;
};

struct Item {
    int type;
    int respawn_time;
    int timeSinceConsumption;
    Item *next;
    Hitbox *hitbox;
};


struct Trap {
    int cooldown_time;
    int timeSinceActivated;
    int type;
    int timeInEffect; // time since trap has been activated
    int effectTime; // duration of the trap's effect
    Hitbox *effectHitbox;
    Hitbox *activateHitbox;
    Trap * next;
};


Hitbox *makeHitbox(int offset_x, int offset_y, int width, int height) {
    Hitbox *newHitBox = (Hitbox*) malloc(sizeof(Hitbox));

    newHitBox->offset_x = offset_x;
    newHitBox->offset_y = offset_y;
    newHitBox->width = width;
    newHitBox->height = height;

    return newHitBox;  
}

// - function to check if the the hitboxes of 2 entities collapse
// - input: pointer to hitbox 1, pointer to hitbox 2
// - output: 1 if 2 entities collapse, 0 otherwise
// - dependencies:
int check_collision(Hitbox *a, Hitbox *b) {

    return 0;
}

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

// ------------------Class Game and related functions used only by Game defined ------------------

struct Player {
    int id; // id of player
    int udp_port_byte_sent = 0; // the amount of bytes that have been sent to the client regarding the UDP port message
    int socket_descriptor; // socket descriptor corresponding for each player (to handle select())
    int ready;
    sockaddr_in cliaddr; // IPv4 address corresponding to each player
    Hitbox *selfHitBox; //
    char input_buffer[256]; // input buffer of each player
    int bytes_received;

    Player *next; // next player in the list

    int vertical_velocity;
    int collisionx;
    int collisiony;
    int health;
    int char_type;
    int stamina;
    int action;
    int attack_cooldown;
    int dash_cooldown ;
    int is_blocking;
    int is_jumping;
    int is_falling;
    int Hit;
    int timeSinceAttack;
    int timeSinceDash;
    int speed;
    int movement_x;
    int isFacingLeft;
    int posx,posy;
    int timeSinceDeath;
};
struct Game{
    Player* players;
    int game_mode;
    Item* items;
    Trap* traps;
    int game_loop;
    int Score_team1;
    int Score_team2;
    int gravity;
};

void update_player(Player* player, Game *game)
{
    if (player->Hit) player->action = 10;
    player->timeSinceAttack= min(player->timeSinceAttack+1, player->attack_cooldown+1);
    player->timeSinceDash =  min(player->timeSinceDash+1, player->dash_cooldown+1);

    if (player->action==3)
    {
        if (player->movement_x==0)
        {
            player->action=0;
            return;
        }
        else if (player->movement_x==2 && (player->collisionx!=2 && player->collisionx!=3)) 
        {
            player->posx -= player->speed;
            player->posx = max(player->posx, 0);
            player->isFacingLeft = true;
        }
        else if (player->movement_x==1 && (player->collisionx!=1 && player->collisionx!=3) )
        {
            player->posx +=player->speed;
            player->posx = min(player->posx, 65000); 
            player->isFacingLeft= false;
        }
    }
    else if (player->action==0)
    {
        if (player->movement_x==1)
        {
        player->action = 3;
        return;
        }
        else if (player->movement_x==2)
        {
            player->action =3;
            return;
        }
    }

    // update posy
    if (player->collisiony!=2 && player->collisiony !=3) player->posy +=game->gravity;
    printf("%d - %d\n",player->collisionx,player->collisiony);
    printf("%d - %d\n",player->posx, player->posy);
    // finished updating the action now, let's update the position
}
int serialize_player_info(char *send_buffer, int byteSerialized, Player *player)
{
    send_buffer[byteSerialized] =  player->id;
    byteSerialized++;

    send_buffer[byteSerialized] = 0;
    byteSerialized++;

    send_buffer[byteSerialized] = player->char_type;
    byteSerialized++;

    send_buffer[byteSerialized] = player->posx/256;
    byteSerialized++;
    send_buffer[byteSerialized] =  player->posx%256;
    byteSerialized++;

    send_buffer[byteSerialized] = player->posy/256;
    byteSerialized++;
    send_buffer[byteSerialized] =  player->posy%256;
    byteSerialized++;

    if (player->isFacingLeft) 
    {
        send_buffer[byteSerialized]=  1;
        byteSerialized++;
    }
    else 
    {
        send_buffer[byteSerialized]=  0;
        byteSerialized++;
    }

    send_buffer[byteSerialized]=  player->action;
    byteSerialized++;

    send_buffer[byteSerialized]=  player->health;
    byteSerialized++;

    send_buffer[byteSerialized]=  player->stamina;
    byteSerialized++;

    return byteSerialized;
}
void characterSpawner(Player* players)
{
    Player* temp;
    for (temp = players;temp!=NULL;temp=temp->next)
    {
        if (temp->health==0) // This character just died or just got into the game, needs to be spawned
        {
            if (temp->posx==-1 && temp->posy==-1)
            {
                temp->posx = 300;
                temp->posy = 252;
                temp->health=100;
                temp->stamina = 100;
                temp->isFacingLeft=true;
                temp->speed = 3;
                temp->action =  0;
                temp->attack_cooldown = 5;
                temp->dash_cooldown = 5;
                temp->is_blocking=false; 
                temp->is_jumping=false;
                temp->timeSinceAttack=0;
                temp->timeSinceDash=0;
                temp->movement_x=0;  
                temp->char_type=0;        
                temp->is_falling = 0; 
                temp->Hit = 0;
                temp->timeSinceDeath=0;
            }
        }        
    }    
}
void handleStateChange(Player *player, int input_action,int collisionx, int collisiony)
{
    if (player->char_type==0) //class samurai
    {
        if (player->action==input_action) return; // no change so skip
        else 
        {
            if (player->action ==0 || player->action==3 || player->action==8) // characters is in idle state , for now we are considering only class samurai
            {
                if (player->is_jumping+ player->is_falling + player->Hit==0)
                {
                    if (input_action==1 && player->timeSinceAttack>player->attack_cooldown)
                    {
                        player->action =1;
                    } 
                    else if (input_action==2 && player->timeSinceAttack>player->attack_cooldown)
                    {
                        player->action=2;
                    }
                    else if (input_action==3)
                    {
                        player->action = 8;
                    }
                    else if (input_action==4 && player->timeSinceDash >player->dash_cooldown){
                        player->action = 9;                      
                    }
                }

            }  
            else  // does not interrupt other action
            {
                return;
            }          
        }

    }
}

//linked list of items
//each item is at different pos on the screen

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

//--------------------------------------------------------------------------------------------
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
    if (offset!=10)
    {
        for (int i =offset;i<=10;i++) result[i]= 0;
    }
    return 10;
}

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
    p->ready = 0;
    p->health=0;
    p->posx = -1; // just enter the game
    p->posy = -1; // just enter the game
    p->selfHitBox = NULL;
    p->bytes_received = 0;
    p->collisionx = 0;
    p->collisiony = 0;
    p->udp_port_byte_sent=0;

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

// - make a new Trap based on the information provided
// - input: type of trap, effect hitbox, activation hitbox, all other values will be set to 0 or NULL
// - output: pointer to a new Player
// - dependencies: none
// - NOTE: type = 1 is fire trap, type = 2 is slow trap
Trap *makeTrap(int cooldown_time, int type, int effectTime, Hitbox *effectHitbox, Hitbox *activateHitbox){
    Trap *p = (Trap *) malloc(sizeof(Trap));
    p->cooldown_time = cooldown_time;
    p->effectTime = effectTime;
    p->timeSinceActivated = 0; // first init will be 0
    p->timeInEffect = 0;
    p->type = type;
    p->effectHitbox = effectHitbox;
    p->activateHitbox = activateHitbox;
    

    p->next = NULL;

    return p;
}

// - add a trap to list of traps in the game
// - input: head pointer to list of traps in the game, pointer to a Trap
// - output: head of list of traps
// - dependencies: none
Trap *addTrapToListOfTraps(Trap *head, Trap *trap){
    // if there are no traps yet
    if(head == NULL){
        head = trap;
        return head;
    }

    // else add to the end of the linked list
    Trap *p = head;
    while(p->next != NULL){
        p = p->next;
    }

    p->next = trap;

    return head;
}

// - function to remove a trap from the list of traps 
// - input: head pointer to the list of traps, pointer to the trap to remove
// - output: updated head pointer of the list of traps
Trap *removeTrapFromListOfTraps(Trap *head, Trap *removeTrap){
    // if list of traps empty
    if(head == NULL){
        return NULL;
    }

    // if we need to remove head
    if(head == removeTrap){
        head = head->next;
        free(removeTrap);

        // printf("List of traps updated (trap to remove was at the head of list)\n");
        return head;
    }

    // traverse to find previous node 
    Trap *temp = head;
    while(temp != NULL && temp->next != removeTrap){
        temp = temp->next;
    }

    // if the player to remove is not found
    if(temp == NULL){
        // printf("Trap not found in the list, list unchanged\n");
        return head;
    }

    // remove the node
    temp->next = removeTrap->next;
    free(removeTrap);
    // printf("List of traps updated (trap to remove was not at head of the list)\n");
    return head;
}

// - function to update information of a trap
// - input: pointer to a trap to update
// - output: none
// - dependencies: none
void updateTrapInformation(Trap *trap) {
    // update time sice last activated
    trap->timeSinceActivated++; // simply increment time since last activated

    // update time in effect
    trap->timeInEffect = min(trap->timeInEffect + 1, trap->effectTime);
}

// - function to check if a player has stepped on an activated trap
// - input: pointer to player, pointer to trap
// - output: 0 if the trap is not activated by the player, type of trap if trap is activated
// - dependencies: check_collision
int trapActivated(Player *player, Trap *trap){
    // check if trap is still on cool down
    if(trap->timeSinceActivated < trap->cooldown_time) return 0;

    // if player doesn't step on the trap
    if(!check_collision(player->selfHitBox, trap->activateHitbox)) return 0;

    // else update trap information and return trap's type
    trap->timeSinceActivated = 0;
    trap->timeInEffect = 0; // this means trap is now in effect

    return trap->type;
}

// - function to check if a player affected when a trap is activated
// - input: pointer to a player, pointer to the trap
// - output: 0 if player is not affected by a trap, type of trap if player is affected by the trap
int takeEffectFromTrap(Player *player, Trap *trap) {
    // check if player is not in the trap effect's area
    if(!check_collision(player->selfHitBox, trap->effectHitbox)) return 0;

    // check if trap is not in effect anymore
    if(trap->timeInEffect >= trap->effectTime) return 0;
    
    return trap->type;
}

// - function to generate all traps of a map
// - input: none
// - output: pointer to the head of linked list of traps
// - dependencies: makeTrap(), makeHitbox(), addTrapToListOfTraps()
Trap *spawnAllTraps(){
    // init head pointer of list of traps
    Trap *traps = NULL;

    // create all traps in this map

    // create first trap
    Hitbox *firstTrapEffectHitbox = makeHitbox(800, 336, 16, 16);
    Hitbox *firstTrapActivateHitbox = makeHitbox(200, 336, 16, 16);
    Trap *firstTrap = makeTrap(50, 1, 20, firstTrapEffectHitbox, firstTrapActivateHitbox);

    // add first trap to list of traps
    traps = addTrapToListOfTraps(traps, firstTrap);

    // create second trap
    Hitbox *secondTrapEffectHitbox = makeHitbox(1300, 150, 16, 16);
    Hitbox *secondTrapActivateHitbox = makeHitbox(1500, 336, 16, 16);
    Trap *secondTrap = makeTrap(50, 2, 20, secondTrapEffectHitbox, secondTrapActivateHitbox);

    // add second trap to list of traps
    traps = addTrapToListOfTraps(traps, secondTrap);

    return traps;
}


// - make a new item based on the information provided
// - input: type of item, hitbox, all other values will be set to 0 or NULL
// - output: pointer to a new item
// - dependencies: none
Item *makeItem(int type, int respawn_time, int timeSinceConsumption, Hitbox *hitbox){
    Item *p = (Item *) malloc(sizeof(Item));
    p->type = type;
    p->respawn_time = respawn_time;
    p->timeSinceConsumption = timeSinceConsumption;
    p->hitbox = hitbox;
    return p;
}

// - add a trap to list of items in the game
// - input: head pointer to list of items in the game, pointer to a Items
// - output: head of list of items
// - dependencies: none
Item *addItemToListOfItems(Item *head, Item *trap){
    // if there are no items yet
    if(head == NULL){
        head = trap;
        return head;
    }

    // else add to the end of the linked list
    Item *p = head;
    while(p->next != NULL){
        p = p->next;
    }

    p->next = trap;

    return head;
}

// - function to remove a item from the list of items 
// - input: head pointer to the list of items, pointer to the item to remove
// - output: updated head pointer of the list of items
Item *removeItemFromListOfItems(Item *head, Item *removeItem){
    // if list of items empty
    if(head == NULL){
        return NULL;
    }

    // if we need to remove head
    if(head == removeItem){
        head = head->next;
        free(removeItem);

        // printf("List of items updated (item to remove was at the head of list)\n");
        return head;
    }

    // traverse to find previous node 
    Item *temp = head;
    while(temp != NULL && temp->next != removeItem){
        temp = temp->next;
    }

    // if the item to remove is not found
    if(temp == NULL){
        // printf("Item not found in the list, list unchanged\n");
        return head;
    }

    // remove the node
    temp->next = removeItem->next;
    free(removeItem);
    // printf("List of items updated (item to remove was not at head of the list)\n");
    return head;
}

// - function to update information of a trap
// - input: pointer to a trap to update
// - output: none
// - dependencies: none
void updateItemInformation(Item *item) {
    // update time sice last activated
    item->timeSinceConsumption++; // simply increment time since last activated
}




// - Verify player eligibility for item consumption
// - input: player
// - output: type of item (0 if the player does not has ability to consume any items)
int consumedItem(Player *player, Item *head){
    Item *p = head;

    // if list of item is empty
    if(p==NULL) return 0;

    // Traverse to find satisfy item
    while(p!=NULL){
        if(p->timeSinceConsumption > p->respawn_time && check_collision(player->selfHitBox, p->hitbox)){
            p->timeSinceConsumption = 0;
            return p->type;
        }
        p=p->next;
    }

    // Return 0 if the player does not has ability to consume any items
    return 0;
}

// - function to generate all items of a map
// - input: none
// - output: pointer to the head of linked list of items
// - dependencies: makeItems(), makeHitbox(), addItemToListOfItems()
Item* spawnAllItems() {
    // init head pointer of list of items
    Item* items = NULL;

    // create all items in this map
    Hitbox* item1EffectHitbox = makeHitbox(800, 200, 16, 16);
    Item* item1 = makeItem(1, 50, 51, item1EffectHitbox);
    items = addItemToListOfItems(items, item1);

    Hitbox* item2EffectHitbox = makeHitbox(800, 220, 16, 16);
    Item* item2 = makeItem(2, 50, 51, item2EffectHitbox);
    items = addItemToListOfItems(items, item2);

    Hitbox* item3EffectHitbox = makeHitbox(800, 240, 16, 16);
    Item* item3 = makeItem(3, 50, 51, item3EffectHitbox);
    items = addItemToListOfItems(items, item3);

    Hitbox* item4EffectHitbox = makeHitbox(800, 260, 16, 16);
    Item* item4 = makeItem(4, 50, 51, item4EffectHitbox);
    items = addItemToListOfItems(items, item4);

    Hitbox* item5EffectHitbox = makeHitbox(800, 280, 16, 16);
    Item* item5 = makeItem(5, 50, 51, item5EffectHitbox);
    items = addItemToListOfItems(items, item5);

    return items;
}
