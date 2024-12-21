DROP DATABASE IF EXISTS game_users;

-- create database
CREATE DATABASE game_users;

-- connect to this database 
\c game_users;

-- create table
CREATE TABLE users (
    id SERIAL PRIMARY KEY,
    username VARCHAR(50) NOT NULL,
    password VARCHAR(50) NOT NULL,
    games_played INT DEFAULT 0,
    score INT DEFAULT 0
);

-- add constraint to table
ALTER TABLE users 
ADD CONSTRAINT users_unique_username UNIQUE(username);

-- init data
INSERT INTO users (username, password) VALUES ('hieu', 'hieu', 15, 20);
INSERT INTO users (username, password) VALUES ('duc', 'duc', 2, 1);
INSERT INTO users (username, password) VALUES ('hoa', 'hoa');
INSERT INTO users (username, password) VALUES ('hoang', 'hoang', 3, 5);
INSERT INTO users (username, password) VALUES ('abc', 'abc');