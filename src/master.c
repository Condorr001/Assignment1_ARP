#include "constants.h"
#include "wrapFuncs/wrapFunc.h"
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

static void spawn(char **arg_list) { Execvp(arg_list[0], arg_list); }

int main(int argc, char *argv[]) {
    // define the number of programs to spawn
    int num_children = MASTER_CHILDS;

    // define an array of strings for every process to spawn
    char programs[num_children][20];
    strcpy(programs[0], "./server");
    strcpy(programs[1], "./WD");
    strcpy(programs[2], "./drone");
    strcpy(programs[3], "./input");

    // Pids for all children
    pid_t childs[num_children];

    // pipe for the communication between input and drone
    int ItoD[2];

    // value for waiting for the children to terminate
    int res;

    // create the pipe and print the ids in a string
    Pipe(ItoD);
    char ItoD_string[80];
    sprintf(ItoD_string, "%d %d", ItoD[0], ItoD[1]);

    for (int i = 0; i < num_children; i++) {
        int pid = Fork();
        if (!pid) {
            // spawn the first two programs, which don't need pipes
            if (i < num_children / 2) {
                char *arg_list[] = {programs[i], NULL};
                spawn(arg_list);
            }

            // spawn the last two programs, which need pipes
            if (i < num_children) {
                char *arg_list[] = {programs[i], ItoD_string, NULL};
                spawn(arg_list);
            }
        } else {
            // wait for all children to terminate
            for (int i = 0; i < num_children; i++) {
                int ret = Wait(&res);
                int status;
                WEXITSTATUS(status);
                printf("Process %d terminated with code: %d\n", ret, status);
            }
        }
    }

    return 0;
}
