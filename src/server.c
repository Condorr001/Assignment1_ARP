#include <stdio.h> 
#include <string.h> 
#include <fcntl.h> 
#include <sys/stat.h> 
#include <sys/types.h> 
#include <sys/select.h>
#include <unistd.h> 
#include <stdlib.h>
#include "wrapFuncs/wrapFunc.h"
#include "constants.h"
#include <semaphore.h>
#include <sys/mman.h>

sem_t *Sem_open(const char *name, int flag, mode_t mode, unsigned int value) {
    sem_t *ret_val = sem_open(name, flag, mode, value);

    if (ret_val == SEM_FAILED) {
        perror("sem_open()");
        fflush(stdout);
        getchar();
        exit(1);
    }

    return ret_val;
}

int Sem_init(sem_t *sem_id, int pshared, int init) {
    int ret_val = sem_init(sem_id, pshared, init);

    if (ret_val < 0) {
        perror("sem_init()");
        fflush(stdout);
        getchar();
        exit(1);
    }

    return ret_val;
}

int Shm_open(const char *name, int flag, mode_t mode) {
    int ret_val = shm_open(name, flag, mode);
    if (ret_val < 0) {
        perror("shm_open()");
        fflush(stdout);
        getchar();
        exit(1);
    }
    return ret_val;
}

int Ftruncate(int fd, __off_t length) {
    int ret_val = ftruncate(fd, length);

    if (ret_val < 0) {
        perror("ftruncate()");
        fflush(stdout);
        getchar();
        exit(1);
    }

    return ret_val;
}

void *Mmap(void *address, size_t length, int protocol, int flags, int fd, off_t offset) {
    void *ret_val = mmap(address, length, protocol, flags, fd, offset);

    if (ret_val == MAP_FAILED) {
        perror("mmap()");
        fflush(stdout);
        getchar();
        exit(1);
    }

    return ret_val;
}

int Sem_wait(sem_t *sem) {
    int ret_val = sem_wait(sem);
    if (ret_val < 0) {
        perror("sem_wait()");
        fflush(stdout);
        getchar();
        exit(EXIT_FAILURE);
    }
    return ret_val;
}

int Sem_post(sem_t *sem){
    int ret_val = sem_post(sem);
    if (ret_val < 0) {
        perror("sem_post()");
        fflush(stdout);
        getchar();
        exit(EXIT_FAILURE);
    }
    return ret_val;
}

int Sem_close(sem_t *sem){
    int ret_val = sem_close(sem);
    if (ret_val < 0) {
        perror("sem_close()");
        fflush(stdout);
        getchar();
        exit(1);
    }
    return ret_val;
}

int Sem_unlink(const char *name){
    int ret_val = sem_unlink(name);
    if (ret_val < 0) {
        perror("sem_unlink()");
        fflush(stdout);
        getchar();
        exit(1);
    }
    return ret_val;
}


int main(int argc, char *argv[]) 
{
    // initialize semaphore
    sem_t * sem_id = Sem_open(SEM_PATH, O_CREAT, S_IRUSR | S_IWUSR, 1);
    Sem_init(sem_id, 1, 0); //initialized to 0 until shared memory is instantiated

    //here goes what is shared with the drone (where is it, direction of movement etc.)
    //My idea is to have a shared string where the drone writes its state, so that the server can read it and write it in the logfile
    //So something like:
    char status[MAX_STRING_LEN];
    int shared_seg_size = strlen(status);

    // create shared memory object
    int shm = Shm_open(SHMOBJ_PATH, O_CREAT | O_RDWR, S_IRWXU | S_IRWXG);

    // truncate size of shared memory
    Ftruncate(shm, shared_seg_size);

    // map pointer
    void* shm_ptr = Mmap(NULL, shared_seg_size, PROT_READ | PROT_WRITE, MAP_SHARED, shm, 0);

    // post semaphore
    sem_post(sem_id);

    // every two seconds write in the logfile the state of the drone (it would be better to write only if the status has changed)
    while (1) 
    {      
        Sem_wait(sem_id);
        memcpy(status, shm_ptr, shared_seg_size);
        //write in the logfile
        Sem_post(sem_id);
        
        sleep(2);
    } 

    // clean up
    shm_unlink(SHMOBJ_PATH);
    Sem_close(sem_id);
    Sem_unlink(SEM_PATH);
    munmap(shm_ptr, shared_seg_size);

    return 0; 
} 
