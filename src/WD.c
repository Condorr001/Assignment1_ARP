#include "constants.h"
#include "utils/utils.h"
#include "wrapFuncs/wrapFunc.h"
#include <fcntl.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/file.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

// Defining the amount to sleep between any two consequent signals to the
// processes
#define SLEEP_FOR 1

// Array for processes' pids
int p_pids[NUM_PROCESSES + 2];

// Pid of the konsole executing input and the map processes
int pid_konsole_input, pid_konsole_map;

// count to check whether process has replied to WD
int count = 0;

// Check of kill
int check;

// pid of the dead process
int fault_pid;

// signal handler for signals received from monitored processes
void signal_handler(int signo, siginfo_t *info, void *context) {
    if (signo == SIGUSR2)
        count++;
}

int main(int argc, char *argv[]) {
    // signal setup
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));

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
    sprintf(filename_string, "log/log.log");

    // argv[1] -> server
    // argv[2] -> drone
    // argv[3] -> konsole of input
    if (argc == NUM_PROCESSES) {
        for (int i = 0; i < 2; i++)
            sscanf(argv[i + 1], "%d", &p_pids[i + 2]);
        sscanf(argv[3], "%d", &pid_konsole_input);
    } else {
        perror("arg_list");
        exit(1);
    }


    // getting the input pid through the named pipe
    //initializing the named pipe
    int fd1;
    char *fifo_two = "/tmp/fifo_two"; //same as in input
    Mkfifo(fifo_two, 0666);

    //string to save the input pid
    char input_pid_str[10];

    //opening the named pipe and reading the input pid from it
    fd1 = Open(fifo_two, O_RDONLY);
    Read(fd1, input_pid_str, sizeof(input_pid_str));
    Close(fd1);

    //saving the input pid in the array of processes pids
    sscanf(input_pid_str, "%d", &p_pids[0]);

    //printing the received input pid to verify its correctness
    printf("\ninput pid is %s\n", input_pid_str);

    // getting both the map pid and the pid of the Konsole running map through the named pipe
    //initializing the named pipe
    int fd2;
    char *fifo_one = "/tmp/fifo"; //same as in map
    Mkfifo(fifo_one, 0666);

    //string to save both pids
    char map_pids_str[20];

    //opening the named pipe and reading the pids from it
    fd2 = Open(fifo_one, O_RDONLY);
    Read(fd2, map_pids_str, sizeof(map_pids_str));
    Close(fd2);

    //printing the received pids to verify their correctness
    printf("Map pids are %s\n", map_pids_str);

    // save map pid in the array of processes pids and the pid of the Konsole running map in a separate int variable.
    //This because the Konsole(s) do not receive nor send signals to the WD, but only need to be killed if
    //a process freezes or dies
    sscanf(map_pids_str, "%d|%d", &p_pids[1], &pid_konsole_map);

    while (1) {
        //iterate for the number of processes to check
        for (int i = 0; i < NUM_PROCESSES; i++) {
            //saving the return value of the kill so that it is possible to immediately verify
            //if the kill failed, so if a process died
            check = Kill2(p_pids[i], SIGUSR1);

            // Writing in the logfile which process the WD sent a signal to
            FILE *F;
            F = Fopen(filename_string, "a");
            //locking the logfile since also the server can write into it
            Flock(fileno(F), LOCK_EX);
            fprintf(F, "[INFO] - Wd sending signal to %d\n", p_pids[i]);
            //unlocking the file so that the server can access it again
            Flock(fileno(F), LOCK_UN);
            fclose(F);

            // This may seem strange but is required because signals may
            // interrupt sleep. Sleep when interrupted returns the amount of
            // seconds for which it still needed to sleep for. So this is
            // actually restarting the sleep in case it does not return 0. Now
            // it may seem a busy wait but it's actually not because of how
            // signals are handled
            while (sleep(SLEEP_FOR))
                ;

            //if either the process is dead, so the kill failed returning -1 to check,
            //or the process is frozen, so it did not increment the count, the WD
            //kills all processes and exits
            if (check == -1 || count == 0) {
                //saving the pid of the dead/frozen process in a specific variable
                fault_pid = p_pids[i];

                //opening the logfile
                FILE *F0;
                F0 = Fopen(filename_string, "a");
                //locking the logfile since also the server can write into it
                Flock(fileno(F0), LOCK_EX);
                //writing in logfile the pid of the guilty process
                fprintf(
                    F0,
                    "[WARN] - WD killed all processes because of process with "
                    "pid %d\n",
                    fault_pid);
                //unlocking the file so that the server can access it again
                Flock(fileno(F0), LOCK_UN);
                fclose(F0);

                //killing all processes, except Konsole's
                for (int i = 0; i < NUM_PROCESSES; i++) {
                    Kill2(p_pids[i], SIGKILL);
                }

                ///killing Konsole's
                Kill2(pid_konsole_input, SIGKILL);
                Kill2(pid_konsole_map, SIGKILL);

                //exiting with success
                return 0;
            }
            //if the process is not dead nor frozen, reset its count
            else
                count = 0;
        }
    }
    return 0;
}
