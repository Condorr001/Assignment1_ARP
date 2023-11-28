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

//signal handler to receive the signal from WD and reply with another signal
void signal_handler(int signo, siginfo_t *info, void *context) {
    if (signo == SIGUSR1) {
        //get the WD pid since its the sender of the signal
        WD_pid = info->si_pid;
        Kill(WD_pid, SIGUSR2);
    }
}

int main(int argc, char *argv[]) {
    // signal setup
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));

    sa.sa_sigaction = signal_handler;
    sigemptyset(&sa.sa_mask);
    // SA_RESTART has been used to restart all those syscalls that can get
    // interrupted by signals
    sa.sa_flags = SA_SIGINFO | SA_RESTART;

    //linking the signal handler to the signal
    if (sigaction(SIGUSR1, &sa, NULL) < 0) {
        perror("SIGUSR1: sigaction()");
        exit(1);
    }

    //forking so that the father can behave as the server, while the child can spawn the map
    int pid = Fork();

    if (pid) {
        // Father process
        // initialize semaphores for each drone information
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

        // create shared memory object
        int shm = Shm_open(SHMOBJ_PATH, O_CREAT | O_RDWR, S_IRWXU | S_IRWXG);

        // truncate size of shared memory
        Ftruncate(shm, MAX_SHM_SIZE);

        // map pointer
        void *shm_ptr = Mmap(NULL, MAX_SHM_SIZE, PROT_READ | PROT_WRITE,
                             MAP_SHARED, shm, 0);
        memset(shm_ptr, 0, MAX_SHM_SIZE);

        //structs for each drone information
        struct pos drone_current_pos;
        struct velocity drone_current_velocity;
        struct force drone_current_force;

        // logfile
        char filename_string[80];
        sprintf(filename_string, "log/log.log");
        FILE *F0;

        // every two seconds write in the logfile the state of the drone
        while (1) {
            //taking the semaphores so other processes cannot access them
            Sem_wait(sem_force);
            Sem_wait(sem_velocity);
            Sem_wait(sem_position);

            //saving drone information (position, velocity and force) from shared memory
            sscanf(shm_ptr + SHM_OFFSET_POSITION, "%f|%f", &drone_current_pos.x,
                   &drone_current_pos.y);
            sscanf(shm_ptr + SHM_OFFSET_VELOCITY_COMPONENTS, "%f|%f",
                   &drone_current_velocity.x_component,
                   &drone_current_velocity.y_component);
            sscanf(shm_ptr + SHM_OFFSET_FORCE_COMPONENTS, "%f|%f",
                   &drone_current_force.x_component,
                   &drone_current_force.y_component);

            // write all the info in the logfile
            F0 = Fopen(filename_string, "a");
            //locking the logfile since also the WD can write into it
            Flock(fileno(F0), LOCK_EX);
            //printing the drone info in the logfile
            fprintf(
                F0,
                "[INFO] - The x-y position of the drone is: %f %f\n"
                "[INFO] - The x-y components of the drone velocity are: %f %f\n"
                "[INFO] - The x-y components of the drone force are: %f %f\n",
                drone_current_pos.x, drone_current_pos.y,
                drone_current_velocity.x_component,
                drone_current_velocity.y_component,
                drone_current_force.x_component,
                drone_current_force.y_component);

            //unlocking the file so that the WD can access it again
            Flock(fileno(F0), LOCK_UN);
            fclose(F0);

            //releasing the semaphores
            Sem_post(sem_position);
            Sem_post(sem_velocity);
            Sem_post(sem_force);

            //sleep 2 seconds so that the server write in the logfile every two seconds
            sleep(2);
        }

        // clean up

        //unlink the shared memory
        shm_unlink(SHMOBJ_PATH);
        //close the semaphores
        Sem_close(sem_velocity);
        Sem_close(sem_force);
        Sem_close(sem_position);
        //unlink the semaphores
        Sem_unlink(SEM_PATH_FORCE);
        Sem_unlink(SEM_PATH_POSITION);
        Sem_unlink(SEM_PATH_VELOCITY);
        //unmap the shared memory
        munmap(shm_ptr, MAX_SHM_SIZE);

        return 0;

    } else {
        // passing the required arguments to the map process and spawn it
        char *args[] = {"konsole", "-e", "./map", NULL};
        Execvp("konsole", args);
    }
}
