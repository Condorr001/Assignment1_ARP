#include "constants.h"
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

// Count to check whether process has replied to WD
int count = 0;

// Check of kill
int check;

// Pid of the dead process
int fault_pid;

// Signal handler for signals received from the monitored processes
void signal_handler(int signo, siginfo_t *info, void *context) {
    // Specifying that context and info are unused
    (void)(info);
    (void)(context);

    if (signo == SIGUSR2)
        count++;
}

int main(int argc, char *argv[]) {
    // Signal declaration
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));

    // Setting the signal handler
    sa.sa_sigaction = signal_handler;
    sigemptyset(&sa.sa_mask);
    // Setting flags
    // The SA_RESTART flag is used to restart all those syscalls that can get
    // interrupted by signals
    sa.sa_flags = SA_SIGINFO | SA_RESTART;

    // Enabling the handler with the specified flags
    Sigaction(SIGUSR2, &sa, NULL);

    // argv[1] -> server
    // argv[2] -> drone
    // argv[3] -> konsole of input
    if (argc == NUM_PROCESSES) {
        for (int i = 0; i < 2; i++)
            sscanf(argv[i + 1], "%d", &p_pids[i + 2]);
        sscanf(argv[3], "%d", &pid_konsole_input);
    } else {
        perror("arg_list error");
        exit(1);
    }

    // Getting the input pid through the named pipe
    // Initializing the named pipe
    int fd1;
    Mkfifo(FIFO2_PATH, 0666);

    // String to save the input pid
    char input_pid_str[10];

    // Opening the named pipe and reading the input pid from it
    fd1 = Open(FIFO2_PATH, O_RDONLY);
    Read(fd1, input_pid_str, sizeof(input_pid_str));
    Close(fd1);

    // Saving the input pid in the array of processes pids
    sscanf(input_pid_str, "%d", &p_pids[0]);

    // Printing the received input pid to verify its correctness
    printf("\nInput pid is %s\n", input_pid_str);

    // Getting both the map pid and the pid of the Konsole running map through
    // the named pipe
    // Initializing the named pipe
    int fd2;
    Mkfifo(FIFO1_PATH, 0666);

    // String to save both pids
    char map_pids_str[20];

    // Opening the named pipe and reading the pids from it
    fd2 = Open(FIFO1_PATH, O_RDONLY);
    Read(fd2, map_pids_str, sizeof(map_pids_str));
    Close(fd2);

    // Save map pid in the array of processes pids and the pid of the Konsole
    // running map in a separate int variable.
    // This because the Konsole(s) do not receive nor send signals to the WD,
    // but only need to be killed if a process freezes or dies
    sscanf(map_pids_str, "%d|%d", &p_pids[1], &pid_konsole_map);

    // Printing the received pids to verify their correctness
    printf("Map pid is %d and the konsole terminal on which it runs is %d \n",
           p_pids[1], pid_konsole_map);

    while (1) {
        // Iterate for the number of processes to check
        for (int i = 0; i < NUM_PROCESSES; i++) {
            // Saving the return value of the kill so that it is possible to
            // immediately verify if the kill failed, so if a process died
            check = Kill2(p_pids[i], SIGUSR1);

            // Writing in the logfile which process the WD sent a signal to
            FILE *F;
            F = Fopen(LOGFILE_PATH, "a");
            // Locking the logfile since also the server can write into it
            Flock(fileno(F), LOCK_EX);
            fprintf(F, "[INFO] - Wd sending signal to %d\n", p_pids[i]);
            // Unlocking the file so that the server can access it again
            Flock(fileno(F), LOCK_UN);
            Fclose(F);

            // This may seem strange but is required because signals may
            // interrupt sleep. Sleep when interrupted returns the amount of
            // seconds for which it still needed to sleep for. So this is
            // actually restarting the sleep in case it does not return 0. Now
            // it may seem a busy wait but it's actually not because of how
            // signals are handled
            while (sleep(SLEEP_FOR))
                ;

            // If either the process is dead, so the kill failed returning -1 to
            // check, or the process is frozen, so it did not increment the
            // count, the WD kills all processes and exits
            if (check == -1 || count == 0) {
                // Saving the pid of the dead/frozen process in a specific
                // variable
                fault_pid = p_pids[i];

                // Opening the logfile
                FILE *F0;
                F0 = Fopen(LOGFILE_PATH, "a");
                // Locking the logfile since also the server can write into it
                Flock(fileno(F0), LOCK_EX);
                // Writing in logfile the pid of the guilty process
                fprintf(
                    F0,
                    "[WARN] - WD killed all processes because of process with "
                    "pid %d\n",
                    fault_pid);
                // Unlocking the file so that the server can access it again
                Flock(fileno(F0), LOCK_UN);
                Fclose(F0);

                // Killing all processes, except Konsole's
                for (int i = 0; i < NUM_PROCESSES; i++) {
                    Kill2(p_pids[i], SIGKILL);
                }

                /// Killing Konsole's
                Kill2(pid_konsole_input, SIGKILL);
                Kill2(pid_konsole_map, SIGKILL);

                // Exiting with success
                return EXIT_SUCCESS;
            }
            // if the process is not dead nor frozen, reset its count
            else
                count = 0;
        }
    }
    return EXIT_SUCCESS;
}
