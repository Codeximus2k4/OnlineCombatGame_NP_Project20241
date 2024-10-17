# Makefile
# How to use: (type into terminal)
# make all: compile all C source files into binaries
# make clean: clean all C binaries

# Rule to build executables
all: server
	@echo "\nAll C code compile into binaries\n"; 
	@echo "*****Online combat game*****";
	@echo "How to play: "; 
	@echo "1. Start C server"; 
	@echo "2. Start Python client"; 

server: server.cpp
	@echo "Compiling C server code..."; 
	@g++ -o server server.cpp;

# Clean rule to remove the executable
clean:
	@echo "\nCleaning all C binaries..."; 
	@rm -rf server;
	@echo "Completed cleaning";