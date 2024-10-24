## Connect C server to Postgresql database


>Note: Giu nguyen ten database, ten table, attributes trong file duoi day de code chay muot. Neu thay doi ten thi thay doi ten tuong ung trong code** 

### I. Set up postgresql

#### 1. Install PostgreSQL
If PostgreSQL is not installed yet, you can install it as follows:

Open terminal, type:
```
sudo apt update
sudo apt install postgresql postgresql-contrib
```

#### 2. Switch to the PostgreSQL User
By default, PostgreSQL creates a system user named postgres. Switch to this user to configure the database:
```
sudo -i -u postgres
```
To exit from this user, type
```
exit
```

#### 3. Access the PostgreSQL Command Line
Once you are the postgres user, access the PostgreSQL command line interface (CLI) by typing:
```
psql
```

- note: after successfully connected to postgres, run script in _./db-data/db-init.sql_ to initialize database (need to know absolute path to _/db-data/db-init.sql_)
```
\i abosulute/path/to/script
```

#### 4. Create a Database
Now that you’re inside the PostgreSQL shell, create a new database:
```
CREATE DATABASE users;
```
You can verify that the database was created by listing all databases:
```
\l
```
exit viewing by 
```
q
```
#### 5. Create a Table in the Database
Next, create a table inside your database. First, connect to the database:
```
\c users
```
Create the table
```
CREATE TABLE users (
    id SERIAL PRIMARY KEY,
    username VARCHAR(50) UNIQUE NOT NULL,
    password VARCHAR(50) NOT NULL
);
```

List tables
``` 
\dt
```

#### 6. Insert Sample Data
Insert some sample data into the table:
```
INSERT INTO users (username, password) VALUES ('ducngo', '123456'), (hihi123, 8386);
```
You can verify the data by querying the table:
```
SELECT * FROM users;
```
7. Exit from postgresql cli, type
```
\q
```





### II. Set up connection

#### 1. Install  libpq

libpq is the official C application programming interface (API) for PostgreSQL. It allows you to connect, interact, and perform various operations on a PostgreSQL database from a C program.

Make sure that libpq library are installed on your system. You can install them using your package manager:
```
sudo apt-get install libpq-dev
```
#### 2. Run server: authen_server.cpp file
```
gcc -o authen_server authen_server.cpp
```
If u face the error: <libpq-fe.h> not found, then try:
```
gcc -o authen_server authen_server.cpp -I/usr/include/postgresql -L/usr/lib -lpq
```
The reason u face this error is that postgresql is not stored in the standard location. U must find where it is located (in my case is /use/lib),then put it after the L flag

then compile
```
./authen_server
```
U are required to fill in the username and password. U should fill in "postgres" for username and its corresponding password

#### 3. Run client: game_process.py and try to login/register
After you register successfully, u can check the database users to see the changes


