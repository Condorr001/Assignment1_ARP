#ifndef WRAPFUNC
#define WRAPFUNC
#include <semaphore.h>
#include <stddef.h>
#include <stdio.h>
#include <sys/select.h>
#include <sys/file.h>

int Wait(int *wstatus);
int Waitpid(pid_t pid, int *wstatus, int options);
int Execvp(const char* file, char** args);
int Read(int fd, void *buf, size_t nbytes);
int Write(int fd, void *buf, size_t nbytes);
int Fork();
int Select(int nfds, fd_set *readfds, fd_set *writefds, fd_set *exceptfds,
           struct timeval *timeout);
int Open(const char *file, int oflag);
int Pipe(int *pipedes);
int Close(int fd);
int Shm_open(const char *name, int flag, __mode_t mode);
int Ftruncate(int fd, __off_t length);
void *Mmap(void *addr, size_t len, int prot, int flags, int fd, __off_t offset);
sem_t *Sem_open(const char *name, int oflag, mode_t mode, unsigned int value);
int Sem_init(sem_t *sem, int pshared, int value);
int Sem_wait(sem_t *sem);
int Sem_post(sem_t *sem);
int Sem_close(sem_t *sem);
int Sem_unlink(const char *name);
FILE* Fopen(const char* pathname, const char* mode);
int Flock(int fd, int operation);
int Kill(int pid, int signal);
#endif // !WRAPFUNC
