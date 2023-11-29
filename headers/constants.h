#ifndef CONSTANTS_H
#define CONSTANTS_H

#define NUM_PROCESSES 4

#define SHMOBJ_PATH "/shm_server"
#define SEM_PATH_POSITION "/sem_position"
#define SEM_PATH_FORCE "/sem_force"
#define SEM_PATH_VELOCITY "/sem_velocity"
#define LOGFILE_PATH "log/log.log"
#define FIFO1_PATH "/tmp/fifo_one"
#define FIFO2_PATH "/tmp/fifo_two"

#define MAX_SHM_SIZE 1024
#define SHM_OFFSET_POSITION 0
#define SHM_OFFSET_FORCE_COMPONENTS 100
#define SHM_OFFSET_VELOCITY_COMPONENTS 200

#define SIMULATION_WIDTH 500
#define SIMULATION_HEIGHT 500

#endif // !CONSTANTS_H
