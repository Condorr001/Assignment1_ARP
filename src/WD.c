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

#define DT 4 // Time period for timeout in seconds

//number of processes to monitorate
int num_processes = NUM_PROCESSES;

//array for P's pids
int p_pids[3];

//pid of the konsole executing input
int pid_konsole_input;

//string for input pid
char input_pid_str[10];

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

    //path of the pid file
    char filename2_string[80];
    sprintf(filename2_string, "../file/pid.txt");

    if (argc == num_processes+1) {
        for (int i = 0; i < num_processes-1; i++)
            sscanf(argv[i + 1], "%d", &p_pids[i]);
        sscanf(argv[3], "%d", &pid_konsole_input);
    } 
    else {
        perror("arg_list");
        exit(1);
    }

    //WD is faster than input. Therefore, without usleep, it reads the previous pid written by input in the pid.txt file
    usleep(1000000);
    //read from pid file
    FILE *F1;
    F1 = fopen(filename2_string, "r");
    fgets(input_pid_str, sizeof(input_pid_str), F1);
    sscanf(input_pid_str, "%d", &p_pids[2]);
    fclose(F1);

    printf("\ninput pid is %s\n", input_pid_str);

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
                    kill(pid_konsole_input, SIGKILL);

                    return 0;
                }
            }
            start_time = current_time;
        }
        sleep(10);
    }

    return 0;
