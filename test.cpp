#include <stdio.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <stdlib.h>

int main(void){
    int shmid;
    key_t key;
    pid_t pid;
    char *s_addr, *p_addr;

    shmid = shmget(key, 1024, 0777 | IPC_CREAT);
    if (shmid < 0){
        printf("shmget is error\n");
        return -1;
    }
    printf("shmget is ok and shmid is %d\n", shmid);

    pid = fork();
    if (pid > 0){
        p_addr = (char *) shmat(shmid, NULL, 0);
        strncpy(p_addr, "hello", 5);

        p_addr = (char *) shmat(shmid, NULL, 0);

        p_addr = (char *) shmat(shmid, NULL, 0);
        p_addr = (char *) shmat(shmid, NULL, 0);
        p_addr = (char *) shmat(shmid, NULL, 0);
        p_addr = (char *) shmat(shmid, NULL, 0);
        p_addr = (char *) shmat(shmid, NULL, 0);
        p_addr = (char *) shmat(shmid, NULL, 0);
        
        wait(NULL);

        // Detach from the shared memory
        if (shmdt(p_addr) == -1) {
            perror("shmdt failed");
            exit(1);
        }

        // Remove the shared memory
        if (shmctl(shmid, IPC_RMID, NULL) == -1) {
            perror("shmctl failed to delete shared memory");
    }
    }
    if (pid == 0){
        sleep(2);

        s_addr = (char *) shmat(shmid, NULL, 0);
        printf("s_addr is %s\n", s_addr);
        s_addr = (char *) shmat(shmid, NULL, 0);
        s_addr = (char *) shmat(shmid, NULL, 0);
        s_addr = (char *) shmat(shmid, NULL, 0);
        s_addr = (char *) shmat(shmid, NULL, 0);
        s_addr = (char *) shmat(shmid, NULL, 0);

        printf("s_addr after multiple shmat is %s\n", s_addr);

        // Detach from the shared memory
        if (shmdt(s_addr) == -1) {
            perror("shmdt failed");
            exit(1);
        }
        exit(0);
    }


    return 0;
}