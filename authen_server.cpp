#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#include <libpq-fe.h> //library to connect to postgresql

#define PORT 8080
#define BUFFER_SIZE 1024

void handle_client(int client_socket, PGconn *conn);
int check_user(const char *username, const char *password, PGconn *conn);
int add_user(const char *username, const char *password, PGconn *conn);
void DBconnection();

int main() {
    //////////////////////DB connection///////////////////////////

    //input db infomation
    char host[20], port[5], dbname[20], user[20], password[20];

    printf("Enter user (enter postgres for default user): "); //u can use default user = postgres
    scanf("%s", user);
    printf("Enter password: ");
    scanf("%s", password); 

    // Connect to the database
    // conninfo is a string of keywords and values separated by spaces.
    char conninfo[100];
    
    strcpy(conninfo, "host=localhost port=5432 dbname=users "); 
    strcat(conninfo, " user="); 
    strcat(conninfo, user);
    strcat(conninfo, " password=");
    strcat(conninfo, password);
    
    //char conninfo[100 ]= "host=localhost port=5432 dbname=users user=postgres password=@Ngo1702";

    // Create a connection
    PGconn *conn = PQconnectdb(conninfo);

    // Check if the connection is successful
    if (PQstatus(conn) != CONNECTION_OK) {
        // If not successful, print the error message and finish the connection
        printf("Error while connecting to the database server: %s\n", PQerrorMessage(conn));

        // Finish the connection
        PQfinish(conn);

        // Exit the program
        exit(1);
    }

    // We have successfully established a connection to the database server
    printf("Connection Established\n");
    printf("Port: %s\n", PQport(conn));
    printf("Host: %s\n", PQhost(conn));
    printf("DBName: %s\n", PQdb(conn));



    ///////////////////////////socket//////////////////

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
        handle_client(client_socket, conn);
        close(client_socket);
    }

    close(server_fd);

     // Close the connection and free the memory
    PQfinish(conn);

    return 0;
}

void handle_client(int client_socket, PGconn *conn) {
    char buffer[BUFFER_SIZE] = {0};
    char username[50], password[50], operation[10];

    // Read data sent by the client
    read(client_socket, buffer, BUFFER_SIZE);

    // Parse the operation, username, and password
    sscanf(buffer, "%s\n%s\n%s", operation, username, password);

    if (strcmp(operation, "LOGIN") == 0) {
        if (check_user(username, password, conn)) {
            send(client_socket, "Login Successfully\n", strlen("Login Successfully\n"), 0);
        } else {
            send(client_socket, "Invalid Username or Password\n", strlen("Invalid Username or Password\n"), 0);
        }
    } else if (strcmp(operation, "REGISTER") == 0) {
        if (add_user(username, password, conn)) {
            send(client_socket, "Register Successfully\n", strlen("Register Successfully\n"), 0);
        } else {
            send(client_socket, "Username Already Exists\n", strlen("Username Already Exists\n"), 0);
        }
    }
}

int check_user(const char *username, const char *password, PGconn *conn) {
    // SQL query to check for user credentials
    const char *query = "SELECT * FROM users WHERE username = $1 AND password = $2";
    const char *paramValues[2];
    paramValues[0] = username; // User input for username
    paramValues[1] = password; // User input for password (ideally should be hashed)

    // Execute the query
    PGresult *res = PQexecParams(conn, query, 2, NULL, paramValues, NULL, NULL, 0);

    // Check for errors in query execution
    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        fprintf(stderr, "SELECT query failed: %s\n", PQerrorMessage(conn));
        PQclear(res);
        return 0; // Query failed
    }

    // If the query returns 1 tuple, user found
    int found = (PQntuples(res) == 1) ? 1 : 0;

    // Clean up
    PQclear(res);
    
    return found;
}


int add_user(const char *username, const char *password, PGconn *conn) {
    // Check if username exists
    const char *checkQuery = "SELECT * FROM users WHERE username = $1";
    const char *paramValues[1];
    paramValues[0] = username; // User input parameter

    PGresult *res = PQexecParams(conn, checkQuery, 1, NULL, paramValues, NULL, NULL, 0);

    // Check for errors in the SELECT query
    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        fprintf(stderr, "SELECT query failed: %s\n", PQerrorMessage(conn));
        PQclear(res);
        return 0;
    }

    // If the query returns 1 tuple, user exists
    if (PQntuples(res) == 1) {
        PQclear(res);
        return 0; // User already exists
    }

    // User doesn't exist; add new user
    const char *insertQuery = "INSERT INTO users (username, password) VALUES ($1, $2)";
    const char *insertParams[2];
    insertParams[0] = username;
    insertParams[1] = password; // Ideally, this should be hashed!

    PGresult *insertRes = PQexecParams(conn, insertQuery, 2, NULL, insertParams, NULL, NULL, 0);

    // Check if the INSERT execution was successful
    if (PQresultStatus(insertRes) != PGRES_COMMAND_OK) {
        fprintf(stderr, "INSERT query failed: %s\n", PQerrorMessage(conn));
        PQclear(insertRes);
        PQclear(res); // Clear the previous SELECT result
        return 0; // Insert failed
    }

    // Clean up
    PQclear(insertRes);
    PQclear(res);
    
    return 1; // User added successfully
}

