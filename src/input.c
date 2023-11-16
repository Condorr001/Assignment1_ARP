#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <time.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "constants.h"
#include "wrapFuncs/wrapFunc.h"

//WD pid
pid_t WD_pid;

void signal_handler (int signo, siginfo_t *info, void *context) {
    if (signo == SIGUSR1) {
        sleep(2);
        WD_pid = info->si_pid;
        kill(WD_pid, SIGUSR2);
    }
}

int main(int argc, char *argv[]) {

    //signal setup
    struct sigaction sa;
    //memset(&sa, 0, sizeof(sa));

    sa.sa_sigaction = signal_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_SIGINFO;

    if(sigaction(SIGUSR1, &sa, NULL) < 0) {
        perror("SIGUSR1: sigaction()");
        exit(1);
    }

    while (1) {}

    return 0;
}