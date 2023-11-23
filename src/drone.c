#include "constants.h"
#include "dataStructs.h"
#include "wrapFuncs/wrapFunc.h"
#include <fcntl.h>
#include <math.h>
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

void signal_handler(int signo, siginfo_t *info, void *context) {
    if (signo == SIGUSR1) {
        sleep(1);
        WD_pid = info->si_pid;
        kill(WD_pid, SIGUSR2);
    }
}

float repulsive_force(float distance, float function_scale, float area_of_effect) {
    // this function returns the border effect given the general
    // function given in the docs folder of the project
    return function_scale * (area_of_effect - distance) / (distance / function_scale);
}

int main(int argc, char *argv[]) {
    // signal setup
    struct sigaction sa;
    // memset(&sa, 0, sizeof(sa));

    // Assigning the signal handler
    sa.sa_sigaction = signal_handler;
    // Resetting the mask
    sigemptyset(&sa.sa_mask);
    // Setting flags
    sa.sa_flags = SA_SIGINFO;

    if (sigaction(SIGUSR1, &sa, NULL) < 0) {
        perror("SIGUSR1: sigaction()");
        exit(1);
    }

    // Initializing structs to store data gotten and given by this process
    // drone_force as every other of these structs has a x and a y component
    // Drone force is the force applied to the drone by the input
    struct force drone_force;
    // walls is the force acting on the drone due to the close distance to
    // the walls
    struct force walls;
    // drone_current_position stores the current position of the drone
    struct pos drone_current_position;
    // drone_current_velocity stores the curretn velocity of the drone
    struct velocity drone_current_velocity;

    // Read the parameters for the border effect
    // Function scale determines the slope of the function while
    // area of effect for how many meters will the border repel the object
    float function_scale = get_param("drone", "function_scale");
    float area_of_effect = get_param("drone", "area_of_effect");

    // Read a first time from the paramter file to set the constants
    float M = get_param("drone", "mass");
    float T = get_param("drone", "time_step");
    float K = get_param("drone", "viscous_coefficient");

    // Initializing the variables that will store the position at time
    // t-1 and t-2. So xt_1 means x(t-1) and xt_2 x(t-2). The same goes
    // for y variable
    float xt_1, xt_2;
    float yt_1, yt_2;

    // Setting the structs declared above with defauls values
    // Initializing the position of the drone with the parameters specified
    // in the parameters file
    drone_current_position.x = xt_1 = xt_2 = get_param("drone", "init_pos_x");
    drone_current_position.y = yt_1 = yt_2 = get_param("drone", "init_pos_y");
    // The force and the velocity applied to the drone at t = 0 is 0
    drone_force.x_component = 0;
    drone_force.y_component = 0;
    drone_current_velocity.x_component = 0;
    drone_current_velocity.y_component = 0;

    // Here the number of cycles to wait before reading again from file is
    // read from the paramaters file
    int reading_rate_reductor = get_param("drone", "reading_rate_reductor");

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
        // If reading_rate_reductor is equal to 0 is time to read again from the
        // parameters file
        if (!reading_rate_reductor--) {
            // The first parameter to be read is the reading_rate_reductor
            // itself.
            reading_rate_reductor = get_param("drone", "reading_rate_reductor");
            // Then all the other phisic parameters are read
            M = get_param("drone", "mass");
            T = get_param("drone", "time_step");
            K = get_param("drone", "viscous_coefficient");
            function_scale = get_param("drone", "function_scale");
            area_of_effect = get_param("drone", "area_of_effect");
        }
        // The semaphore is taken in order to read the force components as
        // given by the user in the input process
        Sem_wait(sem_id);
        sscanf(shm_ptr + SHM_OFFSET_FORCE_COMPONENTS, "%f|%f",
               &drone_force.x_component, &drone_force.y_component);
        Sem_post(sem_id);

        // Calculating repulsive force from sides
        // note that in the docs the image of the function is provided and it
        // shows that only after 'area_of_effect' in the x axis it will start to
        // work. This explains the constraint in the if. So if xt_1 is less than
        // 'area_of_effect' distance from the any wall in the x axis it will be
        // affeccted by the force
        if (xt_1 < area_of_effect) {
            walls.x_component =
                repulsive_force(xt_1, function_scale, area_of_effect);
            // In the following if the right edge is checked
        } else if (xt_1 > SIMULATION_WIDTH - area_of_effect) {
            walls.x_component = -repulsive_force(SIMULATION_WIDTH - xt_1, function_scale, area_of_effect);
        }
        if (yt_1 < area_of_effect) {
            walls.y_component = repulsive_force(yt_1, function_scale, area_of_effect);
            // In the following if the bottom edge is checked
        } else if (yt_1 > SIMULATION_HEIGHT - area_of_effect) {
            walls.y_component = -repulsive_force(SIMULATION_HEIGHT - yt_1, function_scale, area_of_effect);
        }

        // Here the current position of the drone is calculated using
        // the provided formula. The x and y components are calculated
        // using the same formula
        drone_current_position.x =
            (walls.x_component + drone_force.x_component -
             (M / (T * T)) * (xt_2 - 2 * xt_1) + (K / T) * xt_1) /
            ((M / (T * T)) + K / T);
        drone_current_position.y =
            (walls.y_component + drone_force.y_component -
             (M / (T * T)) * (yt_2 - 2 * yt_1) + (K / T) * yt_1) /
            ((M / (T * T)) + K / T);

        // The current velocity is calculated by dividing the
        // difference of position by the time step between the calculations
        // both for the x and y axis
        drone_current_velocity.x_component =
            (drone_current_position.x - xt_1) / T;
        drone_current_velocity.y_component =
            (drone_current_position.y - yt_1) / T;

        // Here the time dependant values are updated
        // Every iteration a time step T is elapsed so:
        xt_2 = xt_1;
        xt_1 = drone_current_position.x;

        // This is done also for the y axis
        yt_2 = yt_1;
        yt_1 = drone_current_position.y;

        // The calculated position is written in shared memory in order to allow
        // the input process to correctly display it in the ncurses interface.
        // To do that firstly the semaphore needs to be taken
        Sem_wait(sem_id);
        sprintf(shm_ptr + SHM_OFFSET_POSITION, "%.5f|%.5f",
                drone_current_position.x, drone_current_position.y);
        Sem_post(sem_id);
        // The calculated velocity is written in the shared memory in order to
        // allow the input process to correctly display it in the curses
        // interface
        Sem_wait(sem_id);
        sprintf(shm_ptr + SHM_OFFSET_VELOCITY_COMPONENTS, "%f|%f",
                drone_current_velocity.x_component,
                drone_current_velocity.y_component);
        Sem_post(sem_id);

        // The process needs to wait T seconds before computing again the
        // position as specified in the paramaters file. Here usleep needs the
        // amount to sleep for in microsecons. So 1 second in microseconds is
        // given and then multiplied by T to get the correct amount of
        // microseconds to sleep for.
        usleep(1000000 * T);
    }

    // Cleaning up
    shm_unlink(SHMOBJ_PATH);
    Sem_close(sem_id);
    Sem_unlink(SEM_PATH);
    munmap(shm_ptr, MAX_SHM_SIZE);
    return 0;
}
