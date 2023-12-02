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

// Once the SIGUSR1 is received send back the SIGUSR2 signal
void signal_handler(int signo, siginfo_t *info, void *context) {
    // Specifying that context is unused
    (void)(context);

    if (signo == SIGUSR1) {
        WD_pid = info->si_pid;
        Kill(WD_pid, SIGUSR2);
    }
}

int main(int argc, char *argv[]) {
    // Specifying that argc and argv are unused variables
    (void)(argc);
    (void)(argv);

    // Signal declaration
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));

    // Setting the signal handler
    sa.sa_sigaction = signal_handler;
    sigemptyset(&sa.sa_mask);
    // Setting flags
    // The SA_RESTART flag is used to restart all those syscalls that can get
    // interrupted by signals
    sa.sa_flags = SA_SIGINFO | SA_RESTART;

    // Enabling the handler with the specified flags
    Sigaction(SIGUSR1, &sa, NULL);

    // Forking so that the father can behave as the server, while the child can
    // spawn the Map
    int pid = Fork();

    if (pid) {
        // Father process
        // Initialize semaphores for each drone information
        sem_t *sem_force =
            Sem_open(SEM_PATH_FORCE, O_CREAT, S_IRUSR | S_IWUSR, 1);
        sem_t *sem_position =
            Sem_open(SEM_PATH_POSITION, O_CREAT, S_IRUSR | S_IWUSR, 1);
        sem_t *sem_velocity =
            Sem_open(SEM_PATH_VELOCITY, O_CREAT, S_IRUSR | S_IWUSR, 1);

        // Setting the semaphores value to 1
        Sem_init(sem_force, 1, 1);
        Sem_init(sem_position, 1, 1);
        Sem_init(sem_velocity, 1, 1);

        // Create shared memory object
        int shm = Shm_open(SHMOBJ_PATH, O_CREAT | O_RDWR, S_IRWXU | S_IRWXG);

        // Truncate size of shared memory
        Ftruncate(shm, MAX_SHM_SIZE);

        // Map pointer to shared memory area
        void *shm_ptr = Mmap(NULL, MAX_SHM_SIZE, PROT_READ | PROT_WRITE,
                             MAP_SHARED, shm, 0);
        memset(shm_ptr, 0, MAX_SHM_SIZE);

        // Structs for each drone information
        struct pos drone_current_pos;
        struct velocity drone_current_velocity;
        struct force drone_current_force;

        // Declaring the logfile aux buffer
        char logmsg[100];

        while (1) {
            // Taking the semaphores to write into the shared memory areas
            Sem_wait(sem_force);
            Sem_wait(sem_velocity);
            Sem_wait(sem_position);

            // Saving drone information (position, velocity and force) from
            // shared memory
            sscanf((char *)shm_ptr + SHM_OFFSET_POSITION, "%f|%f",
                   &drone_current_pos.x, &drone_current_pos.y);
            sscanf((char *)shm_ptr + SHM_OFFSET_VELOCITY_COMPONENTS, "%f|%f",
                   &drone_current_velocity.x_component,
                   &drone_current_velocity.y_component);
            sscanf((char *)shm_ptr + SHM_OFFSET_FORCE_COMPONENTS, "%f|%f",
                   &drone_current_force.x_component,
                   &drone_current_force.y_component);

            sprintf(logmsg, "The x-y position of the drone is: %f %f",
                    drone_current_pos.x, drone_current_pos.y);
            logging(LOG_INFO, logmsg);
            sprintf(logmsg,
                    "The x-y components of the drone velocity are: %f %f",
                    drone_current_velocity.x_component,
                    drone_current_velocity.y_component);
            logging(LOG_INFO, logmsg);

            sprintf(logmsg,
                    "The x-y components of the drone force are: %f %f",
                    drone_current_force.x_component,
                    drone_current_force.y_component);
            logging(LOG_INFO, logmsg);
            // Releasing the semaphores
            Sem_post(sem_position);
            Sem_post(sem_velocity);
            Sem_post(sem_force);

            // Sleep 2 seconds so that the server write in the logfile every two
            // seconds
            sleep(2);
        }

        /// Clean up
        // Unlinking the shared memory area
        Shm_unlink(SHMOBJ_PATH);
        // Closing the semaphors
        Sem_close(sem_velocity);
        Sem_close(sem_force);
        Sem_close(sem_position);
        // Unlinking the semaphors files
        Sem_unlink(SEM_PATH_FORCE);
        Sem_unlink(SEM_PATH_POSITION);
        Sem_unlink(SEM_PATH_VELOCITY);
        // Unmapping the shared memory pointer
        Munmap(shm_ptr, MAX_SHM_SIZE);
        return EXIT_SUCCESS;

    } else {
        // Passing the required arguments to the map process and spawn it
        char *args[] = {"konsole", "-e", "./map", NULL};
        Execvp("konsole", args);

        // In case we arrive here there is was an error
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
