#include<stdio.h>
#include<string.h>

struct Player {
    int id; // id of player
    char name[50]; // player's name
    int position_x; // player x position
    int position_y; // player y position
    int flip;
    char action[50]; // player's current action
    int frame;
};

char *serializePlayerInfo(Player player){
    char string[10000]; // actual string that stores serialized player data
    char *result; // pointer to string
    char strnum[50]; // used for converting integer to array of char

    result = string;
    
    // append player id
    snprintf(string, sizeof(string), "%d", player.id);
    strcat(string, "|");

    // append player name
    strcat(string, player.name);
    strcat(string, "|");

    // append position x
    snprintf(strnum, sizeof(strnum), "%d", player.position_x);
    strcat(string, strnum);
    strcat(string, "|");

    // append position y
    snprintf(strnum, sizeof(strnum), "%d", player.position_y);
    strcat(string, strnum);
    strcat(string, "|");

    // append player flip
    snprintf(strnum, sizeof(strnum), "%d", player.flip);
    strcat(string, strnum);
    strcat(string, "|");

    // append player action 
    strcat(string, player.action);
    strcat(string, "|");

    // append player flip
    snprintf(strnum, sizeof(strnum), "%d", player.frame);
    strcat(string, strnum);

    return result;
}


int main() {
    int i = 551235;
    char str[1000];
    Player a;
    a.id = 1;
    strcpy(a.name, "hieu");
    a.position_x = 5;
    a.position_y = 8;
    strcpy(a.action, "attacking");
    a.flip = 1;
    a.frame = 1;

    char *result = serializePlayerInfo(a);

    printf("%s\n", result);
    printf("strlen of *result = %d\n", strlen(result));
    

    return 0;
}