#include "constants.h"
#include "wrapFuncs/wrapFunc.h"
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <signal.h>
#include <unistd.h>

//WD pid
pid_t WD_pid;

void signal_handler (int signo, siginfo_t *info, void *context) {
    if (signo == SIGUSR1) {
        WD_pid = info->si_pid;
        kill(WD_pid, SIGUSR2);
    }
}

int main(int argc, char *argv[]) {
    //signal setup
    struct sigaction sa;
    //memset(&sa, 0, sizeof(sa));

    sa.sa_sigaction = signal_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_SIGINFO;

    if(sigaction(SIGUSR1, &sa, NULL) < 0) {
        perror("SIGUSR1: sigaction()");
        exit(1);
    }

    int server_map[2];
    Pipe(server_map);

    int pid = Fork();

    if (pid) {
        // Father process
        // Closing the reading pipe end from the server side
        close(server_map[0]);

        // initialize semaphore
        sem_t *sem_id = Sem_open(SEM_PATH, O_CREAT, S_IRUSR | S_IWUSR, 1);
        Sem_init(sem_id, 1,
                 0); // initialized to 0 until shared memory is instantiated

        // here goes what is shared with the drone (where is it, direction of
        // movement etc.) My idea is to have a shared string where the drone
        // writes its state, so that the server can read it and write it in the
        // logfile So something like:
        char status[MAX_STRING_LEN];
        int shared_seg_size = strlen(status);

        // create shared memory object
        int shm = Shm_open(SHMOBJ_PATH, O_CREAT | O_RDWR, S_IRWXU | S_IRWXG);

        // truncate size of shared memory
        Ftruncate(shm, shared_seg_size);

        // map pointer
        void *shm_ptr = Mmap(NULL, shared_seg_size, PROT_READ | PROT_WRITE,
                             MAP_SHARED, shm, 0);

        // post semaphore
        sem_post(sem_id);

        // every two seconds write in the logfile the state of the drone (it
        // would be better to write only if the status has changed)
        while (1) {
            Sem_wait(sem_id);
            memcpy(status, shm_ptr, shared_seg_size);
            // write in the logfile
            Sem_post(sem_id);
            
            
            // TODO TEMPORANEO
            Write(server_map[1],"123.45|234.23", 14);

            sleep(2);
        }

        // clean up
        shm_unlink(SHMOBJ_PATH);
        Sem_close(sem_id);
        Sem_unlink(SEM_PATH);
        munmap(shm_ptr, shared_seg_size);

        return 0;

    } else {
        
        // closing the writing end of the map side
        close(server_map[1]);

        // converting the pipe number to string
        char pipe1[10];
        sprintf(pipe1, "%d", server_map[0]);

        // passing the required arguments to the map process
        char* args[] = {"konsole", "-e", "./map", pipe1, NULL};
        Execvp("konsole", args);
    }
}
