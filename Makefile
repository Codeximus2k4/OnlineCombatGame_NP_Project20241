# Makefile

# Rule to build executables
all: server

server: server.cpp
	g++ -o server server.cpp

# Clean rule to remove the executable
clean:
	rm -rf server