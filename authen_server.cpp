#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 8080
#define BUFFER_SIZE 1024

void handle_client(int client_socket);
int check_user(const char *username, const char *password);
int add_user(const char *username, const char *password);

int main() {
    int server_fd, client_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);

    // Create socket file descriptor
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Socket failed");
        exit(EXIT_FAILURE);
    }

    // Bind the socket to the port
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("Bind failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    // Listen for incoming connections
    if (listen(server_fd, 3) < 0) {
        perror("Listen failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port %d...\n", PORT);

    // Accept connections from clients
    while ((client_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen)) >= 0) {
        handle_client(client_socket);
        close(client_socket);
    }

    close(server_fd);
    return 0;
}

void handle_client(int client_socket) {
    char buffer[BUFFER_SIZE] = {0};
    char username[50], password[50], operation[10];

    // Read data sent by the client
    read(client_socket, buffer, BUFFER_SIZE);

    // Parse the operation, username, and password
    sscanf(buffer, "%s\n%s\n%s", operation, username, password);

    if (strcmp(operation, "LOGIN") == 0) {
        if (check_user(username, password)) {
            send(client_socket, "Login Successfully\n", strlen("Login Successfully\n"), 0);
        } else {
            send(client_socket, "Invalid Username or Password\n", strlen("Invalid Username or Password\n"), 0);
        }
    } else if (strcmp(operation, "REGISTER") == 0) {
        if (add_user(username, password)) {
            send(client_socket, "Register Successfully\n", strlen("Register Successfully\n"), 0);
        } else {
            send(client_socket, "Username Already Exists\n", strlen("Username Already Exists\n"), 0);
        }
    }
}

int check_user(const char *username, const char *password) {
    FILE *file = fopen("user_database.txt", "r");
    char stored_username[50], stored_password[50];

    if (file == NULL) {
        return 0;
    }

    while (fscanf(file, "%s %s", stored_username, stored_password) != EOF) {
        if (strcmp(username, stored_username) == 0 && strcmp(password, stored_password) == 0) {
            fclose(file);
            return 1; // User found
        }
    }

    fclose(file);
    return 0; // User not found
}

int add_user(const char *username, const char *password) {
    FILE *file = fopen("user_database.txt", "a+");
    char stored_username[50], stored_password[50];

    if (file == NULL) {
        return 0;
    }

    // Check if username already exists
    while (fscanf(file, "%s %s", stored_username, stored_password) != EOF) {
        if (strcmp(username, stored_username) == 0) {
            fclose(file);
            return 0; // Username exists
        }
    }

    // Add new user to the database
    fprintf(file, "%s %s\n", username, password);
    fclose(file);
    return 1; // User added
}
