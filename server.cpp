#include<stdio.h>
#include<sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <arpa/inet.h>
#include<ctype.h>
#include <stdlib.h>

#define BUFF_SIZE 1024 // MAX UDP packet size is 1500 bytes

/*
Data format: data type is in string

id|name|position_x|position_y|flip|action|frame

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
    int frame; // player's frame
    
};

// define variables
Player players[20]; // hold the list of players
int maxPlayer = 0; // keep count of total players in the room

// function to serialize Player info into a string
// input: struct Player
// output: string contains player info
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

// function to publish players information to clients
// input: list of clients addresses
void sendDataToPlayers(Player players[]) {
    
}

// should be called before is_valid_domain()
// check if the provided string is in correct IPv4 format
// other cases can be wrong format or domain name
bool is_valid_ip(char *ip){
    struct sockaddr_in sa;
    return (inet_pton(AF_INET, ip, &(sa.sin_addr)) != 0); 
    // inet_pton() returns 1 if valid IP
}

// check if parameter provided by user is a valid domain name 
// (used after is_valid_ip() is used and parameter is not a valid IP)
// check for TLD domain, if all characters of TLD are not all in alphabetic then false
bool is_valid_domain(char *str){
    // check if user input "localhost"
    if(strcmp(str, "localhost") == 0) return true;

    char check[256];
    strcpy(check, str); // copy a new string so that original provided parameter won't be changed
    char *token;
    char *prevToken; // previous token 

    // initialize token and prevToekn
    token = strtok(check, ".");
    prevToken = token;
    token = strtok(NULL, ".");

    // if provided address doesn't contain a "." then not valid domain
    if(token == NULL){
        return false;
    }

    // now find the TLD characters
    while(token != NULL){
        prevToken = token;
        token = strtok(NULL, ".");
    }

    // now the TLD is stored in prevToken
    // we check if every character in prevToken is alphabetic
    for(int i = 0; i < strlen(prevToken); i++){
        // if a character is a space or a tab then return false
        if(prevToken[i] == '\t' || prevToken[i] == ' ' || prevToken[i] == '\n') return false;
        if(!isalpha(prevToken[i])){
            return false;
        }
    }

    return true;
}

// function to find domain and its aliases (if exists)
// input: ip string
// output: pointer to result string
char *IpToDomainWithAliases(char *ip){
    struct hostent *host;
    struct in_addr ip_addr;
    char **alias;
    char *result; // pointer to the result string
    char string[10000]; // actual result string

    // init
    memset(string, 0, sizeof(string));
    result = string;

    // assign IP address to ip_addr
    if(inet_pton(AF_INET, ip, &ip_addr) != 1){
        strcpy(string, "Wrong IP format\n");
        return result;
    }

    // search for domain name
    host = gethostbyaddr(&ip_addr, sizeof(ip_addr), AF_INET);

    // check if domain name is successfully returned
    if(host == NULL){
        strcpy(string, "Not found information\n");
        return result;
    }

    // append offical domain name
    strcpy(string, "Official name: ");
    strcat(string, host->h_name);
    strcat(string, "\n");

    // check for aliases
    alias = host->h_aliases;
    if(alias == NULL){
        return result;
    }

    // if there are aliases
    strcat(string, "Alias name:\n");
    for(alias = host->h_aliases; *alias != NULL; alias++){
        strcat(string, *alias);
        strcat(string, "\n");
    }

    return result;
}

// function to find IP and its aliases (if exists)
// input: domain string
// output: pointer to result string
char *domainToIp(char *domain) {
    struct addrinfo hints, *res, *p;
    int status;
    char ipstr[INET_ADDRSTRLEN]; // IPv4
    char *result; // char pointer pointing to actual string
    char string[10000]; // actual string 

    // point result pointer to actual string
    result = string;

    // set up hints
    // hints are guidance on what types of addresses the caller is interesting in obtaining
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET; // IPv4
    hints.ai_socktype = SOCK_STREAM; // TCP

    // find this domain name
    // if no IP address found then return
    if( (status = getaddrinfo(domain, NULL, &hints, &res)) != 0 ){
        strcpy(string, "Not found information\n");
        return result;
    }

    // append IP Address
    p = res;
    void *addr;
    struct sockaddr_in *ipv4 = (struct sockaddr_in *) p->ai_addr;
    inet_ntop(AF_INET, &(ipv4->sin_addr), ipstr, sizeof(ipstr));
    strcpy(string, "Official IP: ");
    strcat(string, ipstr);
    strcat(string, "\n");

    // append Alias IP (if exists)
    p = p->ai_next;
    if(p != NULL){
        strcat(string, "Alias IP:\n");
    }

    for(p; p != NULL; p = p->ai_next){
        struct sockaddr_in *ipv4 = (struct sockaddr_in *) p->ai_addr;
        inet_ntop(AF_INET, &(ipv4->sin_addr), ipstr, sizeof(ipstr));
        strcat(string, ipstr);
        strcat(string, "\n");
    }

    // free linked list
    freeaddrinfo(res);

    // return 
    return result;
}


int main (int argc, char *argv[]) {
    int sockfd, rcvBytes, sendBytes;
    char buff[BUFF_SIZE + 1];
    struct sockaddr_in servaddr;
    struct sockaddr_in cliaddr[20]; // list of clients addresses
    socklen_t addr_len = sizeof(struct sockaddr_in); // size of address structure, in this case size of IPv4 structure
    int SERV_PORT;
    char cli_addr[100];
    char *result; // result string to return to client
    char errorString[10000]; // actual string return to client (used in case of error)
    

    // check if user inputed port or not
    if(argc != 2){
        fprintf(stderr, "Usage: ./server port_number\n");
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
    // this loop will receive what is sent from client, and send back what is received
    while(1){
        // reset the buff
        memset(buff, BUFF_SIZE, 0);

        // receive data from client
        rcvBytes = recvfrom(sockfd, buff, BUFF_SIZE, 0, (struct sockaddr *) &cliaddr[0], &addr_len);
        if(rcvBytes < 0){
            perror("Error receiving data from client: ");
            return 0;
        }
        buff[rcvBytes] = '\0';

        // now the last character in buff is \n -> makes the string len + 1, 
        // we need to remove this character
        if(buff[strlen(buff) - 1] == '\n') buff[strlen(buff) - 1] = '\0';

        // store client address into cli_addr variable
        inet_ntop(AF_INET, (void *) &cliaddr[0].sin_addr, cli_addr, INET_ADDRSTRLEN);

        // process data received
        result = buff;

        // print received client address, port and data received
        printf("[%s:%d]: %s\n", cli_addr, ntohs(cliaddr[0].sin_port), buff);

        // send data to client
        sendBytes = sendto(sockfd, result, strlen(result), 0, (struct sockaddr *) &cliaddr[0], addr_len);
        if(sendBytes < 0){
            perror("Error sending data to client: ");
            return 0;
        }
    }

    return 0;
}
