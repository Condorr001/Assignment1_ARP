#include "constants.h"
#include "wrapFuncs/wrapFunc.h"
#include <fcntl.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#define DT 1 // Time period for while loop

// number of processes to monitorate
int num_processes = NUM_PROCESSES+1;

// array for P's pids
int p_pids[4];

// pid of the konsole executing input
int pid_konsole_input;

// string for input pid
char input_pid_str[10];

// initializer for arrived_pids
int count = 0;

//check of kill
int check;

// pid of the dead process
int fault_pid;

// what to do when processes freeze
void signal_handler(int signo, siginfo_t *info, void *context) {
    if (signo == SIGUSR2)
        count++;
}

int main(int argc, char *argv[]) {
    // signal setup
    struct sigaction sa;
    // memset(&sa, 0, sizeof(sa));

    sa.sa_sigaction = signal_handler;
    sigemptyset(&sa.sa_mask);
    // SA_RESTART has been used to restart all those syscalls that can get
    // interrupted by signals
    sa.sa_flags = SA_SIGINFO | SA_RESTART;

    if (sigaction(SIGUSR2, &sa, NULL) < 0) {
        perror("SIGUSR2: sigaction()");
        exit(1);
    }

    // path of the logfile
    char filename_string[80];
    sprintf(filename_string, "../file/log.log");

    // path of the pid file
    char filename2_string[80];
    sprintf(filename2_string, "../file/pid.txt");

    if (argc == num_processes) {
        for (int i = 0; i < num_processes - 1; i++)
            sscanf(argv[i + 1], "%d", &p_pids[i]);
        sscanf(argv[3], "%d", &pid_konsole_input);
    } else {
        perror("arg_list");
        exit(1);
    }

    // WD is faster than input. Therefore, without usleep, it reads nothing in the named pipe
    usleep(1000000);

    //getting the input pid through the named pipe
    int fd1; 
    char * fifo_two = "/tmp/fifo_two"; 
    Mkfifo(fifo_two, 0666); 

    char input_pid_str[10];

    fd1 = Open(fifo_two, O_RDONLY);
    Read(fd1, input_pid_str, sizeof(input_pid_str)); 
    Close(fd1); 

    sscanf(input_pid_str, "%d", &p_pids[2]);

    printf("\ninput pid is %s\n", input_pid_str);

    //getting the map pid through the named pipe
    int fd2; 
    char * fifo_one = "/tmp/fifo"; 
    Mkfifo(fifo_one, 0666); 

    char map_pid_str[10];

    fd2 = Open(fifo_one, O_RDONLY);
    Read(fd2, map_pid_str, sizeof(map_pid_str)); 
    Close(fd2); 

    //save map pid in the array
    sscanf(map_pid_str, "%d", &p_pids[3]);

    while (1) {
        for (int i = 0; i < num_processes; i++) {
            check = Kill2(p_pids[i], SIGUSR1);
            sleep(DT);

            if (check == -1 || count == 0) {
                fault_pid = p_pids[i];
                FILE *F0;
                F0 = Fopen(filename_string, "a");
                fprintf(F0,
                        "WD killed all processes because of process with "
                        "pid %d\n",
                        fault_pid);
                fclose(F0);

                for (int i = 0; i < num_processes; i++)
                    Kill2(p_pids[i], SIGKILL);
                Kill2(pid_konsole_input, SIGKILL);

                return 0;
            }

            else 
                count = 0;
        }
    }
    return 0;
}
