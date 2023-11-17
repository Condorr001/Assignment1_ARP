#include "constants.h"
#include "wrapFuncs/wrapFunc.h"
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

// WD pid
pid_t WD_pid;

struct force {
    float x_component;
    float y_component;
} drone_force;

struct pos {
    float x;
    float y;
} drone_current_position;

void signal_handler(int signo, siginfo_t *info, void *context) {
    if (signo == SIGUSR1) {
        sleep(1);
        WD_pid = info->si_pid;
        kill(WD_pid, SIGUSR2);
    }
}

int main(int argc, char *argv[]) {
    // setup initial constants
    float F = 10;
    float M = 1;
    float T = 0.1;
    float K = 1;
    float xt_1, xt_2;
    float yt_1, yt_2;
    xt_1 = xt_2 = 10;
    yt_1 = yt_2 = 10;
    drone_current_position.x = 10;
    drone_current_position.y = 10;
    drone_force.x_component = 100;
    drone_force.y_component = 100;

    // signal setup
    struct sigaction sa;
    // memset(&sa, 0, sizeof(sa));

    sa.sa_sigaction = signal_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_SIGINFO;

    if (sigaction(SIGUSR1, &sa, NULL) < 0) {
        perror("SIGUSR1: sigaction()");
        exit(1);
    }

    // initialize semaphor
    sem_t *sem_id = Sem_open(SEM_PATH, O_CREAT, S_IRUSR | S_IWUSR, 1);
    // initialized to 0 until shared memory is instantiated
    // Sem_init(sem_id, 1, 0);

    // create shared memory object
    int shm = Shm_open(SHMOBJ_PATH, O_CREAT | O_RDWR, S_IRWXU | S_IRWXG);

    // truncate size of shared memory
    Ftruncate(shm, MAX_SHM_SIZE);

    // map pointer
    void *shm_ptr =
        Mmap(NULL, MAX_SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm, 0);

    while (1) {
        drone_current_position.x =
            (drone_force.x_component - M / (T * T) * (xt_2 - 2 * xt_1) +
             (K / T) * xt_1) /
            (M / (T * T) + K / T);
        drone_current_position.y =
            (drone_force.y_component - M / (T * T) * (yt_2 - 2 * yt_1) +
             (K / T) * yt_1) /
            (M / (T * T) + K / T);

        xt_2 = xt_1;
        xt_1 = drone_current_position.x;

        yt_2 = yt_1;
        yt_1 = drone_current_position.y;

        // write to shm
        Sem_wait(sem_id);
        sprintf(shm_ptr + SHM_OFFSET_POSITION, "%.5f|%.5f",
                drone_current_position.x, drone_current_position.y);
        Sem_post(sem_id);
        sleep(1);
    }

    shm_unlink(SHMOBJ_PATH);
    Sem_close(sem_id);
    Sem_unlink(SEM_PATH);
    munmap(shm_ptr, MAX_SHM_SIZE);
    return 0;
}
