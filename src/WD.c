#include "constants.h"
#include "utils/utils.h"
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

// array for P's pids
int p_pids[NUM_PROCESSES+2];

// pid of the konsole executing input
int pid_konsole_input, pid_konsole_map;

// string for input pid
char input_pid_str[10];

// initializer for arrived_pids
int count = 0;

// check of kill
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

    int sleep_for = get_param("wd", "sleep_for");

    // path of the logfile
    char filename_string[80];
    sprintf(filename_string, "../file/log.log");

    // argv[1] -> server
    // argv[2] -> drone
    // argv[3] -> konsole of input
    if (argc == NUM_PROCESSES) {
        for (int i = 0; i < 2; i++)
            sscanf(argv[i + 1], "%d", &p_pids[i+2]);
        sscanf(argv[3], "%d", &pid_konsole_input);
    } else {
        perror("arg_list");
        exit(1);
    }

    // WD is faster than input. Therefore, without usleep, it reads nothing in
    // the named pipe
    usleep(1000000);

    // getting the input pid through the named pipe
    int fd1;
    char *fifo_two = "/tmp/fifo_two";
    Mkfifo(fifo_two, 0666);

    char input_pid_str[10];

    fd1 = Open(fifo_two, O_RDONLY);
    Read(fd1, input_pid_str, sizeof(input_pid_str));
    Close(fd1);

    sscanf(input_pid_str, "%d", &p_pids[0]);

    printf("\ninput pid is %s\n", input_pid_str);

    // getting the map pid through the named pipe
    int fd2;
    char *fifo_one = "/tmp/fifo";
    //Mkfifo(fifo_one, 0666);

    char map_pids_str[20];

    fd2 = Open(fifo_one, O_RDONLY);
    int ret = Read(fd2, map_pids_str, sizeof(map_pids_str));
    Close(fd2);
    printf("Map pids are %s\n", map_pids_str);

    // save map pid in the array
    sscanf(map_pids_str, "%d|%d", &p_pids[1], &pid_konsole_map);

    while (1) {
        for (int i = 0; i < NUM_PROCESSES; i++) {
            check = Kill2(p_pids[i], SIGUSR1);

            // This may seem strange but is required because signals may
            // interrupt sleep. Sleep when interrupted returns the amount of
            // seconds for witch it still needed to sleep for. So this is
            // actually restarting the sleep in case it does not return 0. Now
            // it may seem a busy wait but it's actually not because of how
            // signals are handled
            while (sleep(sleep_for))
                ;

            if (check == -1 || count == 0) {
                fault_pid = p_pids[i];
                FILE *F0;
                F0 = Fopen(filename_string, "a");
                fprintf(F0,
                        "WD killed all processes because of process with "
                        "pid %d\n",
                        fault_pid);
                fclose(F0);

                for (int i = 0; i < NUM_PROCESSES; i++)
                    Kill2(p_pids[i], SIGKILL);
                Kill2(pid_konsole_input, SIGKILL);
                Kill2(pid_konsole_map, SIGKILL);

                return 0;
            }

            else
                count = 0;
        }
    }
    return 0;
}
