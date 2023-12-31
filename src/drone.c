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

// Once the SIGUSR1 is received send back the SIGUSR2 signal
void signal_handler(int signo, siginfo_t *info, void *context) {
    // Specifying thhat context is not used
    (void)(context);

    if (signo == SIGUSR1) {
        WD_pid = info->si_pid;
        Kill(WD_pid, SIGUSR2);
    }
}

// This function returns the border effect given the general
// function given in the docs folder of the project. All the parameters can be
// modified from the configuration file.
float repulsive_force(float distance, float function_scale,
                      float area_of_effect) {
    return function_scale * (area_of_effect - distance) /
           (distance / function_scale);
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
    // Resetting the mask
    sigemptyset(&sa.sa_mask);
    // Setting flags
    // The SA_RESTART flag has been added to restart all those syscalls that can
    // get interrupted by signals
    sa.sa_flags = SA_SIGINFO | SA_RESTART;

    Sigaction(SIGUSR1, &sa, NULL);

    // Initializing structs to store data gotten and given by this process
    // drone_force as every other of these structs has a x and a y component
    // Drone force is the force applied to the drone by the input
    struct force drone_force;
    // walls is the force acting on the drone due to the close distance to
    // the walls
    struct force walls = {0, 0};
    // drone_current_position stores the current position of the drone
    struct pos drone_current_position = {0, 0};
    // drone_current_velocity stores the curretn velocity of the drone
    struct velocity drone_current_velocity = {0, 0};

    // Read the parameters for the border effect
    // Function scale determines the slope of the function while
    // area of effect determines for how many meters will the border repel the
    // object
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
    drone_force.x_component            = 0;
    drone_force.y_component            = 0;
    drone_current_velocity.x_component = 0;
    drone_current_velocity.y_component = 0;

    // Here the number of cycles to wait before reading again from file is
    // calculated after reading from the paramaters file. To have a better
    // explaination of what's happening in these lines look at the comment of
    // reading_params_interval inside the input.c file
    int reading_params_interval =
        round(get_param("drone", "reading_params_interval") / T);
    // If the counter is less than 1 is set back to 1
    if (reading_params_interval < 1)
        reading_params_interval = 1;

    // Initialize semaphors
    sem_t *sem_position =
        Sem_open(SEM_PATH_POSITION, O_CREAT, S_IRUSR | S_IWUSR, 1);
    sem_t *sem_force = Sem_open(SEM_PATH_FORCE, O_CREAT, S_IRUSR | S_IWUSR, 1);
    sem_t *sem_velocity =
        Sem_open(SEM_PATH_VELOCITY, O_CREAT, S_IRUSR | S_IWUSR, 1);

    // Create shared memory object
    int shm = Shm_open(SHMOBJ_PATH, O_CREAT | O_RDWR, S_IRWXU | S_IRWXG);

    // Truncate size of shared memory
    Ftruncate(shm, MAX_SHM_SIZE);

    // Map pointer to shared memory area
    void *shm_ptr =
        Mmap(NULL, MAX_SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm, 0);

    while (1) {
        // If reading_params_interval is equal to 0 is time to read again from
        // the parameters file
        if (!reading_params_interval--) {
            // The first parameter to be read is the reading_params_interval
            // itself.
            reading_params_interval =
                round((float)get_param("drone", "reading_params_interval") / T);
            // If the counter is less than 1 is set back to 1
            if (reading_params_interval < 1)
                reading_params_interval = 1;

            // Then all the other phisic parameters are read
            M              = get_param("drone", "mass");
            T              = get_param("drone", "time_step");
            K              = get_param("drone", "viscous_coefficient");
            function_scale = get_param("drone", "function_scale");
            area_of_effect = get_param("drone", "area_of_effect");

            // Logging
            logging(LOG_INFO, "Drone has updated its parameters");
        }
        // The semaphore is taken in order to read the force components as
        // given by the user in the input process
        Sem_wait(sem_force);
        sscanf((char *)shm_ptr + SHM_OFFSET_FORCE_COMPONENTS, "%f|%f",
               &drone_force.x_component, &drone_force.y_component);
        Sem_post(sem_force);

        // Calculating repulsive force from the sides.
        // Note that in the docs the image of the function is provided and it
        // shows that only after 'area_of_effect' in the x axis it will start to
        // work. This explains the constraint in the if. So if xt_1 is less than
        // 'area_of_effect' distance from the any wall in the x axis it will be
        // affected by the force
        if (xt_1 < area_of_effect) {
            walls.x_component =
                repulsive_force(xt_1, function_scale, area_of_effect);
            // In the following if the right edge is checked
        } else if (xt_1 > SIMULATION_WIDTH - area_of_effect) {
            walls.x_component = -repulsive_force(
                SIMULATION_WIDTH - xt_1, function_scale, area_of_effect);
        } else {
            walls.x_component = 0;
        }

        if (yt_1 < area_of_effect) {
            walls.y_component =
                repulsive_force(yt_1, function_scale, area_of_effect);
            // In the following if the bottom edge is checked
        } else if (yt_1 > SIMULATION_HEIGHT - area_of_effect) {
            walls.y_component = -repulsive_force(
                SIMULATION_HEIGHT - yt_1, function_scale, area_of_effect);
        } else {
            walls.y_component = 0;
        }

        // Here the current position of the drone is calculated using
        // the provided formula. The x and y components are calculated
        // using the same formula

        // The current velocity is calculated by using floats and there is the
        // possibility that  when this get too small they don't reach 0, so a
        // threshold under which the value is set to zero is set. This threshold
        // has to came into play only when there is no repulsive force from the
        // walls and no force is applied by the user, so when the drone is
        // decelerating only by viscous coefficient. The same applies for
        // the x and y components. It's not possible to directly set velocity to
        // 0 so instead the current position is set equal to the previous one
        if (drone_current_velocity.x_component < ZERO_THRESHOLD &&
            drone_current_velocity.x_component > -ZERO_THRESHOLD &&
            walls.x_component == 0 && drone_force.x_component == 0)
            drone_current_position.x = xt_1;
        else {
            drone_current_position.x =
                (walls.x_component + drone_force.x_component -
                 (M / (T * T)) * (xt_2 - 2 * xt_1) + (K / T) * xt_1) /
                ((M / (T * T)) + K / T);
        }

        if (drone_current_velocity.y_component < ZERO_THRESHOLD &&
            drone_current_velocity.y_component > -ZERO_THRESHOLD &&
            walls.y_component == 0 && drone_force.y_component == 0)
            drone_current_position.y = yt_1;
        else {
            drone_current_position.y =
                (walls.y_component + drone_force.y_component -
                 (M / (T * T)) * (yt_2 - 2 * yt_1) + (K / T) * yt_1) /
                ((M / (T * T)) + K / T);
        }

        // These ifs enforce the boundary of the simulation. These are needed
        // because if the force is set too high it's possible that the next
        // calculated x or y coordinate will completely jump over the border
        // effect. It's also important to notice that, for example when the
        // position is less than 0 it's then set at 1. This to overcome the
        // infinite force that is given by the 0 position because the function
        // that models the repulsive force of the wall has an asymptote in 0.
        // The same goes for all the other walls.
        if (drone_current_position.x > SIMULATION_WIDTH)
            drone_current_position.x = SIMULATION_WIDTH - 1;
        else if (drone_current_position.x < 0)
            drone_current_position.x = 1;
        if (drone_current_position.y < 0)
            drone_current_position.y = 1;
        else if (drone_current_position.y > SIMULATION_HEIGHT)
            drone_current_position.y = 1;

        // The current velocity is calculated by dividing the
        // difference of position by the time step between the calculations
        // both for the x and y axis. This calculation is done here to 
        // always have the most updated velocity calculation in the input
        // Konsole. It delays by one iteration the check of velocity zero,
        // but it's not parcituculary detrimenting to the simulation
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
        // the input process to correctly display it in the ncurses interface,
        // and the map to show the drone on screen. To do that firstly the
        // semaphore needs to be taken
        Sem_wait(sem_position);
        sprintf((char *)shm_ptr + SHM_OFFSET_POSITION, "%.5f|%.5f",
                drone_current_position.x, drone_current_position.y);
        Sem_post(sem_position);
        // The calculated velocity is written in the shared memory in order to
        // allow the input process to correctly display it in the curses
        // interface
        Sem_wait(sem_velocity);
        sprintf((char *)shm_ptr + SHM_OFFSET_VELOCITY_COMPONENTS, "%f|%f",
                drone_current_velocity.x_component,
                drone_current_velocity.y_component);
        Sem_post(sem_velocity);

        // The process needs to wait T seconds before computing again the
        // position as specified in the paramaters file. Here usleep needs the
        // amount to sleep for in microsecons. So 1 second in microseconds is
        // given and then multiplied by T to get the correct amount of
        // microseconds to sleep for.
        usleep(1000000 * T);
    }

    // Cleaning up
    // Unlinking the shared memory area
    Shm_unlink(SHMOBJ_PATH);
    // Closing the semaphors
    Sem_close(sem_position);
    Sem_close(sem_force);
    Sem_close(sem_velocity);
    // Unlinking the semaphors files
    Sem_unlink(SEM_PATH_FORCE);
    Sem_unlink(SEM_PATH_POSITION);
    Sem_unlink(SEM_PATH_VELOCITY);
    // Unmapping the shared memory pointer
    Munmap(shm_ptr, MAX_SHM_SIZE);
    return 0;
}
