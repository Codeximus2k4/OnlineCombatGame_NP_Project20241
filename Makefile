# Makefile
# How to use: (type into terminal)
# make all: compile all C source files into binaries
# make clean: clean all C binaries

# Define color codes
RED    = \033[31m
GREEN  = \033[32m
YELLOW = \033[33m
BLUE   = \033[34m
RESET  = \033[0m

# Rule to build executables
all: server fork_server_test authen_server
	@echo "$(BLUE)*****Online combat game*****$(RESET)";
	@echo "How to play: "; 
	@echo "1. Start C server"; 
	@echo "2. Start Python client"; 
	@echo "$(BLUE)****************************$(RESET)";

server: server.cpp
	@echo "$(GREEN)Compiling C server code...$(RESET)"; 
	@g++ -o server.o server.cpp;

fork_server_test: fork_server_test.cpp
	@echo "$(GREEN)Compiling C fork_server_test...$(RESET)";
	@g++ -o fork_server_test.o fork_server_test.cpp;

authen_server: authen_server.cpp
	@echo  "$(RED)INFO: For compiling authen_server.cpp you need to manually use gcc because of different OS$(RESET)" 

# Clean rule to remove the executable
clean:
	@echo "\n$(GREEN)Cleaning all C binaries..."; 
	@rm -rf server.o;
	@rm -rf fork_server_test.o;
	@rm -rf test.o;
	@rm -rf authen_server.o;
	@rm -rf authen_server;
	@echo "Completed cleaning$(RESET)";