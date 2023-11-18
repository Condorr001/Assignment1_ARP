#include "constants.h"
#include "dataStructs.h"
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
#include <utils/utils.h>

// WD pid
pid_t WD_pid;

// TODO remove from global variables
struct force drone_force;
struct pos drone_current_position;

void signal_handler(int signo, siginfo_t *info, void *context) {
    if (signo == SIGUSR1) {
        sleep(1);
        WD_pid = info->si_pid;
        kill(WD_pid, SIGUSR2);
    }
}

float repulsive_force(float distance) {
    return 10*(20 - distance) / (distance / 10);
}

int main(int argc, char *argv[]) {
    // setup initial constants
    float F = get_param("drone", "force_step");
    float M = get_param("drone", "mass");
    float T = get_param("drone", "time_step");
    float K = get_param("drone", "viscous_coefficient");
    float xt_1, xt_2;
    float yt_1, yt_2;
    struct force walls;
    drone_current_position.x = xt_1 = xt_2 = get_param("drone", "init_pos_x");
    drone_current_position.y = yt_1 = yt_2 = get_param("drone", "init_pos_y");
    drone_force.x_component = 0;
    drone_force.y_component = 0;

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
        // get drone force
        Sem_wait(sem_id);
        sscanf(shm_ptr + SHM_OFFSET_FORCE_COMPONENTS, "%f|%f",
               &drone_force.x_component, &drone_force.y_component);
        Sem_post(sem_id);

        // calculating repulsive force from sides
        // note that in the docs the image of the function is provided and it
        // shows that only after 20 in the x axis it will start to work. This
        // explains the constraint.
        if (xt_1 < 20) {
            walls.x_component = repulsive_force(xt_1);
        }else if(xt_1 > SIMULATION_WIDTH - 20){
            walls.x_component = -repulsive_force(SIMULATION_WIDTH - xt_1);
        }
        if (yt_1 < 20) {
            walls.y_component = repulsive_force(yt_1);
        }else if(yt_1 > SIMULATION_HEIGHT - 20){
            walls.y_component = -repulsive_force(SIMULATION_HEIGHT - yt_1);
        }

        drone_current_position.x =
            (walls.x_component + drone_force.x_component - (M / (T * T)) * (xt_2 - 2 * xt_1) +
             (K / T) * xt_1) /
            ((M / (T * T)) + K / T);
        drone_current_position.y =
            (walls.y_component + drone_force.y_component - (M / (T * T)) * (yt_2 - 2 * yt_1) +
             (K / T) * yt_1) /
            ((M / (T * T)) + K / T);

        // TODO check for boundaries

        xt_2 = xt_1;
        xt_1 = drone_current_position.x;

        yt_2 = yt_1;
        yt_1 = drone_current_position.y;

        // write to shm
        Sem_wait(sem_id);
        sprintf(shm_ptr + SHM_OFFSET_POSITION, "%.5f|%.5f",
                drone_current_position.x, drone_current_position.y);
        Sem_post(sem_id);
        usleep(1000000*T);
    }

    shm_unlink(SHMOBJ_PATH);
    Sem_close(sem_id);
    Sem_unlink(SEM_PATH);
    munmap(shm_ptr, MAX_SHM_SIZE);
    return 0;
}
