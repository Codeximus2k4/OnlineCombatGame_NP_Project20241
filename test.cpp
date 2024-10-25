#include <stdio.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <stdlib.h>

int main() {
    int shmid;
    key_t key;
    pid_t pid;
    char *s_addr, *p_addr;
    char string[50];

    // Create a unique key for the shared memory segment
    key = ftok("./a.c", 'a');
    shmid = shmget(key, 1024, 0777 | IPC_CREAT);

    if (shmid < 0) {
        printf("shmget is error\n");
        return -1;
    }

    printf("shmget is ok and shmid is %d\n", shmid);
    pid = fork();

    if (pid > 0) { // Parent Process
        // Attach to the shared memory segment
        p_addr = (char *) shmat(shmid, NULL, 0);
        
        // Write to shared memory
        strncpy(p_addr, "hello", 6);
        printf("Parent wrote: %s\n", p_addr);

        // Wait for a moment to allow child to read
        sleep(1);

        // Read from shared memory
        printf("Parent read: %s\n", p_addr);

        // Detach from the shared memory
        if (shmdt(p_addr) == -1) {
            perror("shmdt failed");
            exit(1);
        }

        // Delete the shared memory segment
        if (shmctl(shmid, IPC_RMID, NULL) == -1) {
            perror("shmctl failed to delete shared memory");
            exit(1);
        }
    } else if (pid == 0) { // Child Process
        // Attach to the shared memory segment
        s_addr = (char *) shmat(shmid, NULL, 0);
        
        // Read from shared memory
        printf("Child read: %s\n", s_addr);

        // Write to shared memory
        strncpy(s_addr, "child", 5);

        // Detach from the shared memory
        if (shmdt(s_addr) == -1) {
            perror("shmdt failed");
            exit(1);
        }
    }
    return 0;
}
