DROP DATABASE IF EXISTS game_users;

-- create database
CREATE DATABASE game_users;

-- connect to this database 
\c game_users;

-- create table
CREATE TABLE users (
    id SERIAL PRIMARY KEY,
    username VARCHAR(50) UNIQUE NOT NULL,
    password VARCHAR(50) NOT NULL
);

-- init data
INSERT INTO users (username, password) VALUES ('hieu', 'hieu');
INSERT INTO users (username, password) VALUES ('duc', 'duc');
INSERT INTO users (username, password) VALUES ('hoa', 'hoa');
INSERT INTO users (username, password) VALUES ('hoang', 'hoang');