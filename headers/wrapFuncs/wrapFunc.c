#include "wrapFuncs/wrapFunc.h"
#include <fcntl.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <unistd.h>

int Wait(int *wstatus) {
    int ret = wait(wstatus);
    if (ret < 0) {
        perror("Error on wait execution");
        fflush(stdout);
        getchar();
        exit(EXIT_FAILURE);
    }
    return ret;
}

int Waitpid(pid_t pid, int *wstatus, int options){
    int ret = waitpid(pid, wstatus, options);
    if (ret < 0) {
        perror("Error on waitpid execution");
        fflush(stdout);
        getchar();
        exit(EXIT_FAILURE);
    }
    return ret;
}


int Execvp(const char* file, char** args){
    int ret = execvp(file, args);
    if (ret < 0) {
        perror("Error on execvp execution");
        fflush(stdout);
        getchar();
        exit(EXIT_FAILURE);
    }
    return ret;
}

int Fork() {
    int ret = fork();
    if (ret < 0) {
        perror("Error on forking");
        fflush(stdout);
        getchar();
        exit(EXIT_FAILURE);
    }
    return ret;
}

void Write(int fd, char* str, int str_len) {
    ssize_t bytes_written;

    bytes_written = write(fd, str, str_len);
    if (bytes_written < 0) {
        perror("write()");
        exit(0);
    }
}

void Read(int fd, char* msg_received, int msg_length) {
    ssize_t bytes_read;
    bytes_read = read(fd, msg_received, msg_length);

    if (bytes_read < 0) {
        perror("read()");
        exit(0);
    }
}

int Select(int nfds, fd_set *readfds, fd_set *writefds, fd_set *exceptfds,
           struct timeval *timeout) {
    int ret = select(nfds, readfds, writefds, exceptfds, timeout);
    if (ret < 0) {
        perror("Error on executing select");
        fflush(stdout);
        getchar();
        exit(EXIT_FAILURE);
    }
    return ret;
}

int Open(const char *file, int oflag) {
    int ret = open(file, oflag);
    if (ret < 0) {
        perror("Error on executing open");
        fflush(stdout);
        getchar();
        exit(EXIT_FAILURE);
    }
    return ret;
}

int Pipe(int *pipedes) {
    int ret = pipe(pipedes);
    if (ret < 0) {
        perror("Error on executing open");
        fflush(stdout);
        getchar();
        exit(EXIT_FAILURE);
    }
    return ret;
}

int Close(int fd) {
    int ret = close(fd);
    if (ret < 0) {
        perror("Error on executing open");
        fflush(stdout);
        getchar();
        exit(EXIT_FAILURE);
    }
    return ret;
}

int Shm_open(const char *name, int flag, mode_t mode) {
    int ret = shm_open(name, flag, mode);
    if (ret < 0) {
        perror("Error on executing shm_open");
        fflush(stdout);
        getchar();
        exit(EXIT_FAILURE);
    }
    return ret;
}

int Ftruncate(int fd, __off_t length) {
    int ret = ftruncate(fd, length);
    if (ret < 0) {
        perror("Error on executing ftruncate");
        fflush(stdout);
        getchar();
        exit(EXIT_FAILURE);
    }
    return ret;
}

void *Mmap(void *addr, size_t len, int prot, int flags, int fd, off_t offset) {
    void *ret = mmap(addr, len, prot, flags, fd, offset);
    if (ret == MAP_FAILED) {
        perror("Error on executing mmap");
        fflush(stdout);
        getchar();
        exit(EXIT_FAILURE);
    }
    return ret;
}
sem_t *Sem_open(const char *name, int oflag, mode_t mode, unsigned int value) {
    sem_t *ret = sem_open(name, oflag, mode, value);
    if (ret == SEM_FAILED) {
        perror("Error on executing sem_open");
        fflush(stdout);
        getchar();
        exit(EXIT_FAILURE);
    }
    return ret;
}

int Sem_init(sem_t *sem, int pshared, int value) {
    int ret = sem_init(sem, pshared, value);
    if (ret < 0) {
        perror("Error on executing sem_init");
        fflush(stdout);
        getchar();
        exit(EXIT_FAILURE);
    }
    return ret;
}

int Sem_wait(sem_t *sem) {
    int ret = sem_wait(sem);
    if (ret < 0) {
        perror("Error on executing sem_wait");
        fflush(stdout);
        getchar();
        exit(EXIT_FAILURE);
    }
    return ret;
}

int Sem_post(sem_t *sem) {
    int ret = sem_post(sem);
    if (ret < 0) {
        perror("Error on executing sem_post");
        fflush(stdout);
        getchar();
        exit(EXIT_FAILURE);
    }
    return ret;
}

int Sem_close(sem_t *sem) {
    int ret = sem_close(sem);
    if (ret < 0) {
        perror("Error on executing sem_close");
        fflush(stdout);
        getchar();
        exit(EXIT_FAILURE);
    }
    return ret;
}

int Sem_unlink(const char *name) {
    int ret = sem_unlink(name);
    if (ret < 0) {
        perror("Error on executing sem_unlink");
        fflush(stdout);
        getchar();
        exit(EXIT_FAILURE);
    }
    return ret;
}
