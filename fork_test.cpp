#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/wait.h>

int main() {
    int pipe1[2];  // Pipe 1 for Parent -> Child communication
    int pipe2[2];  // Pipe 2 for Child -> Parent communication
    char parentMessage[] = "Hello from parent";
    char childMessage[] = "Hello from child";
    char buffer1[100], buffer2[100];

    // Create pipes
    if (pipe(pipe1) == -1 || pipe(pipe2) == -1) {
        perror("pipe");
        exit(EXIT_FAILURE);
    }

    // Fork a child process
    pid_t pid = fork();

    if (pid < 0) {
        perror("fork");
        exit(EXIT_FAILURE);
    } else if (pid == 0) {
        // This is the child process

        // Close unused ends of pipes
        close(pipe1[1]);  // Close the write end of Pipe 1 (parent -> child)
        close(pipe2[0]);  // Close the read end of Pipe 2 (child -> parent)

		while(1){
			// Read message from parent
			read(pipe1[0], buffer1, sizeof(buffer1));
			read(pipe1[0], buffer2, sizeof(buffer2));
			if(strcmp(buffer1, "*") == 0) break;

			printf("Child received: %s\n", buffer1);
			printf("Child received: %s\n", buffer2);

			// Send message to parent
			write(pipe2[1], childMessage, strlen(childMessage) + 1);
		}
       

        // Close the remaining ends of pipes
        close(pipe1[0]);
        close(pipe2[1]);
    } else {
        // This is the parent process

        // Close unused ends of pipes
        close(pipe1[0]);  // Close the read end of Pipe 1 (parent -> child)
        close(pipe2[1]);  // Close the write end of Pipe 2 (child -> parent)

		while(1){
			fflush(stdin);
			printf("Enter string to send to child: (* to end)");
			scanf("%s", parentMessage);
			if(strcmp(parentMessage, "*") == 0){
				write(pipe1[1], parentMessage, strlen(parentMessage) + 1);
				break;
			}
			// Send message to child
			write(pipe1[1], parentMessage, strlen(parentMessage) + 1);
			write(pipe1[1], parentMessage, strlen(parentMessage) + 1);

			// Read message from child
			read(pipe2[0], buffer1, sizeof(buffer1));
			printf("Parent received: %s\n", buffer1);
		}
        

        // Close the remaining ends of pipes
        close(pipe1[1]);
        close(pipe2[0]);

        // Wait for the child process to finish
        wait(NULL);
    }

    return 0;
}
