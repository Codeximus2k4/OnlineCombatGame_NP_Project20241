#include<stdio.h>
#include<sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <arpa/inet.h>
#include<ctype.h>
#include <stdlib.h>

bool checkSameAddress(sockaddr_in cli1, sockaddr_in cli2){
    if(cli1.sin_family == cli2.sin_family) printf("same family\n");
    if(cli1.sin_port == cli2.sin_port) printf("same port\n");
    if(cli1.sin_addr.s_addr == cli2.sin_addr.s_addr) printf("same IP\n");

    return true;
}

int main() {
    sockaddr_in cli1, cli2;
    cli1.sin_family = AF_INET;
    cli1.sin_port = 5000;

    char ip[50];
    strcpy(ip, "1.1.1.1");

    inet_pton(AF_INET, ip, &cli1.sin_addr);

    cli2.sin_family = AF_INET;
    cli2.sin_port = 5000;
    inet_pton(AF_INET, ip, &cli2.sin_addr);

    check(cli1, cli2);


    return 0;
}