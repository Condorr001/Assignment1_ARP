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
#include <stdbool.h>
#include "constants.h"
#include "wrapFuncs/wrapFunc.h"

#define DT 3 // Time period for timeout in seconds

//number of processes to monitorate
int num_processes = NUM_PROCESSES;

//array for P's pids
int p_pids[3];

//array to save pids of processes which sent a signal
int arrived_pids[3];

//initializer for arrived_pids
int count = 0;

bool switches = true;

//pid of the dead process
int fault_pid;

//what to do when processes freeze
void signal_handler (int signo, siginfo_t *info, void *context) {
    if (signo == SIGUSR2) {
        arrived_pids[count] = info->si_pid;
        count++;
    }
}


int main(int argc, char *argv[]) {
    //signal setup
    struct sigaction sa;
    //memset(&sa, 0, sizeof(sa));

    sa.sa_sigaction = signal_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_SIGINFO;

    if(sigaction(SIGUSR2, &sa, NULL) < 0) {
        perror("SIGUSR2: sigaction()");
        exit(1);
    }

    //path of the logfile
    char filename_string[80];
    sprintf(filename_string, "../file/log.log");

    if (argc == num_processes+1) {
        for (int i = 0; i < num_processes; i++)
            sscanf(argv[i + 1], "%d", &p_pids[i]);
    } 
    else {
        perror("arg_list");
        exit(1);
    }

    //start time for loop period
    time_t start_time = time(NULL);

    while (1) {
        //current time for loop period
        time_t current_time = time(NULL);

        //after DT seconds check if a process is frozen
        if (current_time - start_time >= DT) {
            if(switches) {
                switches = false;

                //initialize arrived_pids to 0
                for (int i = 0; i < num_processes; i++)
                    arrived_pids[i] = 0;

                //reset count
                count = 0;

                //send a signal to each monitored process
                for (int i = 0; i < num_processes; i++)
                    kill(p_pids[i], SIGUSR1);
            }
            else {
                switches = true;

                //if not all processes have sent a signal in response, it means that at least one process is dead
                if (count != num_processes) {
                    FILE *F0;
                    F0 = fopen(filename_string, "a");
                    fprintf(F0, "The arrived pids are: %d %d %d\n", arrived_pids[0], arrived_pids[1], arrived_pids[2]);
                    fclose(F0);

                    for (int i = 0; i < num_processes; i++) {
                        for (int j = 0; j < count; j++) {
                            if (p_pids[i] == arrived_pids[j])
                                break;

                            if (j == count-1)
                                fault_pid = p_pids[i];
                        }
                    }

                    //FILE *F0;
                    F0 = fopen(filename_string, "a");
                    fprintf(F0, "WD killed all processes because of process with pid %d\n", fault_pid);
                    fclose(F0);

                    for(int i = 0; i < num_processes; i++)
                        kill(p_pids[i], SIGKILL);

                    return 0;
                }
            }
            start_time = current_time;
        }
    }

    return 0;
}