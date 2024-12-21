# Makefile
# How to use: (type into terminal)
# make all: compile all C source files into binaries
# make clean: clean all C binaries

#For ubuntu: gcc -o server server.cpp -I/usr/include/postgresql -L/usr/lib -lpq


# brew install postgresql

# GCC command for Mac (Hieu): (after install postgres with brew)

# g++ -I/usr/local/Cellar/libpq/17.0_1/include/ -L/usr/local/Cellar/libpq/17.0_1/lib/ -lpq -o server server.cpp
 
# Define color codes
RED    = \033[31m
GREEN  = \033[32m
YELLOW = \033[33m
BLUE   = \033[34m
RESET  = \033[0m

# Rule to build executables
all: server
	@echo "$(BLUE)*****Online combat game*****$(RESET)";
	@echo "How to play: "; 
	@echo "1. Start C server"; 
	@echo "2. Start Python client"; 
	@echo "$(BLUE)****************************$(RESET)";

server: server/server.cpp
	@echo "$(GREEN)Compiling C server code...$(RESET)"; 
	@g++ -o server.o server/server.cpp -I/usr/include/postgresql -L/usr/lib -lpq

# Clean rule to remove the executable
clean:
	@echo "\n$(GREEN)Cleaning all C binaries..."; 
	@rm -rf server.o;
	@echo "Completed cleaning$(RESET)";