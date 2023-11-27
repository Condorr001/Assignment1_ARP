#include "constants.h"
#include "dataStructs.h"
#include "utils/utils.h"
#include "wrapFuncs/wrapFunc.h"
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

// WD pid
pid_t WD_pid;

void signal_handler(int signo, siginfo_t *info, void *context) {
    if (signo == SIGUSR1) {
        WD_pid = info->si_pid;
        Kill(WD_pid, SIGUSR2);
    }
}

int main(int argc, char *argv[]) {
    // signal setup
    struct sigaction sa;
    // memset(&sa, 0, sizeof(sa));

    sa.sa_sigaction = signal_handler;
    sigemptyset(&sa.sa_mask);
    // SA_RESTART has been used to restart all those syscalls that can get
    // interrupted by signals
    sa.sa_flags = SA_SIGINFO | SA_RESTART;

    if (sigaction(SIGUSR1, &sa, NULL) < 0) {
        perror("SIGUSR1: sigaction()");
        exit(1);
    }

    struct pos drone_current_pos;

    // logfile
    char filename_string[80];
    sprintf(filename_string, "../file/log.log");
    FILE *F0;

    int pid = Fork();

    if (pid) {
        // Father process
        // initialize semaphore
        sem_t *sem_force =
            Sem_open(SEM_PATH_FORCE, O_CREAT, S_IRUSR | S_IWUSR, 1);
        sem_t *sem_position =
            Sem_open(SEM_PATH_POSITION, O_CREAT, S_IRUSR | S_IWUSR, 1);
        sem_t *sem_velocity =
            Sem_open(SEM_PATH_VELOCITY, O_CREAT, S_IRUSR | S_IWUSR, 1);
        // initialized to 0 until shared memory is instantiated
        Sem_init(sem_force, 1, 1);
        Sem_init(sem_position, 1, 1);
        Sem_init(sem_velocity, 1, 1);

        /*
        // here goes what is shared with the drone (where is it, direction of
        // movement etc.) My idea is to have a shared string where the drone
        // writes its state, so that the server can read it and write it in the
        // logfile So something like:
        char status[MAX_SHM_SIZE];
        int shared_seg_size = MAX_SHM_SIZE;
        */

        // create shared memory object
        int shm = Shm_open(SHMOBJ_PATH, O_CREAT | O_RDWR, S_IRWXU | S_IRWXG);

        // truncate size of shared memory
        Ftruncate(shm, MAX_SHM_SIZE);

        // map pointer
        void *shm_ptr = Mmap(NULL, MAX_SHM_SIZE, PROT_READ | PROT_WRITE,
                             MAP_SHARED, shm, 0);
        memset(shm_ptr, 0, MAX_SHM_SIZE);

        // every two seconds write in the logfile the state of the drone (it
        // would be better to write only if the status has changed)
        while (1) {
            Sem_wait(sem_force);
            Sem_wait(sem_velocity);
            Sem_wait(sem_position);
            /*
            memcpy(status, shm_ptr, shared_seg_size);
            printf("%s\t",status);
            printf("%s\n", status+SHM_OFFSET_FORCE_COMPONENTS);
            */
            sscanf(shm_ptr + SHM_OFFSET_POSITION, "%f|%f", &drone_current_pos.x,
                   &drone_current_pos.y);
            // write in the logfile
            F0 = Fopen(filename_string, "a");
            fprintf(F0, "The x-y position of the drone is: %f %f\n",
                    drone_current_pos.x, drone_current_pos.y);
            fclose(F0);
            Sem_post(sem_position);
            Sem_post(sem_velocity);
            Sem_post(sem_force);

            sleep(2);
        }

        // clean up
        shm_unlink(SHMOBJ_PATH);
        Sem_close(sem_velocity);
        Sem_close(sem_force);
        Sem_close(sem_position);
        Sem_unlink(SEM_PATH_FORCE);
        Sem_unlink(SEM_PATH_POSITION);
        Sem_unlink(SEM_PATH_VELOCITY);
        munmap(shm_ptr, MAX_SHM_SIZE);

        return 0;

    } else {
        // passing the required arguments to the map process
        char *args[] = {"konsole", "-e", "./map", NULL};
        Execvp("konsole", args);
    }
}
