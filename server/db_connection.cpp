#include <libpq-fe.h> //library to connect to postgresql
#include<string.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>  // For htons()

//function to establish a connection with the database
//output: db connection
//requirement: 
// - install dependencies: DBConnectionTutorial.md
// - init db: db-data/db-init.sql 
PGconn *connect_db() {
    //////////////////////DB connection///////////////////////////
    // Connect to the database

    char conninfo[100]= "host=localhost port=5432 dbname=game_users user=postgres password=postgres";

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

    return conn;

}

//this function check if an user is in the database or not
//input: username, passsowrd of the user, the db connection
//output: 
// - 1 if the user exists in the db
// - 0 otherwise
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

//this function adds a new user to the database (register)
//input: username, password of the user; the db connection
//output: 
// - 1 if success
// - 0 if fail
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

// Function to get user_id from the database by username
// input: username, db connection
// output 
// - user_id if the user is found
// - -1 if user not found
int get_user_id(const char *username, PGconn *conn) {
    // SQL query to get user_id
    const char *query = "SELECT id FROM users WHERE username = $1";
    const char *paramValues[1];
    paramValues[0] = username;

    // Execute the query
    PGresult *res = PQexecParams(conn, query, 1, NULL, paramValues, NULL, NULL, 0);

    // Check for errors in query execution
    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        fprintf(stderr, "SELECT query failed: %s\n", PQerrorMessage(conn));
        PQclear(res);
        return -1; // Indicate failure to get user_id
    }

    // Check if the user was found
    if (PQntuples(res) != 1) {
        PQclear(res);
        return -1; // User not found
    }

    // Retrieve the user_id from the query result
    int user_id = atoi(PQgetvalue(res, 0, 0));

    // Clean up
    PQclear(res);

    return user_id;
}

// This function closes the DB connection and free the memory
void close_db (PGconn *conn) {

    PQfinish(conn);
}

// - function to update users' statistics to database
// - input: a PGconn connection, player_id, and the score of the match
// - output: 1 if successful, 0 otherwise
int update_player_score(PGconn *conn, int player_id, int score_previous_match) {
    // Convert player_id to a string
    char player_id_str[20];  // Array to hold the string representation of the integer
    snprintf(player_id_str, sizeof(player_id_str), "%d", player_id);

    // Query to get current games_played of this player_id
    const char *query = "SELECT games_played FROM users WHERE id = $1";
    const char *paramValues[1] = {player_id_str}; // pass the string version of player_id

    // Execute the query to get games_played
    PGresult *res = PQexecParams(conn, query, 1, NULL, paramValues, NULL, NULL, 0);
    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        fprintf(stderr, "SELECT query failed (games_played): %s\n", PQerrorMessage(conn));
        PQclear(res);
        return 0; // Failure
    }

    // Retrieve the current games_played value
    int games_played = atoi(PQgetvalue(res, 0, 0));  
    printf("Current games_played for player %d: %d\n", player_id, games_played);

    // Increment this value
    games_played++;

    // Query to get the current score of this player_id
    const char *query_score = "SELECT score FROM users WHERE id = $1";
    PGresult *res_score = PQexecParams(conn, query_score, 1, NULL, paramValues, NULL, NULL, 0);
    if (PQresultStatus(res_score) != PGRES_TUPLES_OK) {
        fprintf(stderr, "SELECT query failed (score): %s\n", PQerrorMessage(conn));
        PQclear(res_score);
        PQclear(res);
        return 0; // Failure
    }

    // Retrieve the current score value
    int score = atoi(PQgetvalue(res_score, 0, 0));  
    printf("Current score for player %d: %d\n", player_id, score);

    // Augment this value with the score of the previous match
    score += score_previous_match;

    // Prepare the update query
    const char *updateQuery = "UPDATE users SET games_played = $1, score = $2 WHERE id = $3";
    char games_played_str[20], score_str[20];
    snprintf(games_played_str, sizeof(games_played_str), "%d", games_played); // Convert to string
    snprintf(score_str, sizeof(score_str), "%d", score); // Convert to string

    const char *updateParams[3] = {games_played_str, score_str, player_id_str};

    // Execute the update query
    PGresult *updateRes = PQexecParams(conn, updateQuery, 3, NULL, updateParams, NULL, NULL, 0);
    if (PQresultStatus(updateRes) != PGRES_COMMAND_OK) {
        fprintf(stderr, "UPDATE query failed: %s\n", PQerrorMessage(conn));
        PQclear(res_score);
        PQclear(res);
        PQclear(updateRes);
        return 0; // Failure
    }

    // Clean up all result sets
    PQclear(res_score);
    PQclear(res);
    PQclear(updateRes);

    return 1; // Success
}

// - Function to get the top 5 players with highest score
// - The data will be populated in `result` string in following format: [username_length][username][games_played][score]x5
// - input: a connection PGconn to postgres, char string to be populated
// - output: number of bytes in `result` string successful, 0 on failure
int get_top_players(PGconn *conn, char result[256]) {
    // Query to get the top 5 players ordered by highest score
    const char *query = "SELECT username, games_played, score FROM users ORDER BY score DESC LIMIT 5";

    // Execute the query
    PGresult *res = PQexec(conn, query);
    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        fprintf(stderr, "SELECT query failed: %s\n", PQerrorMessage(conn));
        PQclear(res);
        return 0;
    }

    // Calculate the total size required for the result string
    int rows = PQntuples(res);
    int total_length = 0;

    // Format each player's data and append to the result string
    for (int i = 0; i < rows; i++) {
        const char *username = PQgetvalue(res, i, 0);
        int username_length = strlen(username);
        const char *games_played_str = PQgetvalue(res, i, 1);
        const char *score_str = PQgetvalue(res, i, 2);

        // Convert games_played and score from string to integer
        int games_played = atoi(games_played_str);
        int score = atoi(score_str);

        // buff to temporary hold data for this player
        char buff[256];

        // set first byte to username_len
        buff[0] = (char) username_length;
        
        // set next bytes as username
        memcpy(buff + 1, username, username_length);

        // set next byte as games_played
        buff[1 + username_length] = (char) games_played;

        // convert score to 2 network bytes
        uint16_t byte_score = htons(score);

        // set these 2 last bytes 
        memcpy(buff + username_length + 2, &byte_score, 2);

        // set last byte of buff as '\0';
        buff[1 + username_length + 1 + 2] = '\0';

        printf("%d-th user: username_len=%d, username=%s, games_played=%d, score=%d\n", i, username_length, username, games_played, score);

        // Append to the result string
        memcpy(result + total_length, buff, 1 + username_length + 1 + 2);

        // update total_length
        total_length = total_length + 1 + username_length + 1 + 2;
    }

    // Clean up
    PQclear(res);

    return total_length;
}

