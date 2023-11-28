#include "constants.h"
#include "wrapFuncs/wrapFunc.h"
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

static void spawn(char **arg_list) {
    Execvp(arg_list[0], arg_list);
    exit(EXIT_FAILURE);
}

int main(int argc, char *argv[]) {
    // define the number of programs to spawn
    int num_children = MASTER_CHILDS;

    // define an array of strings for every process to spawn
    char programs[num_children][20];
    strcpy(programs[0], "./server");
    strcpy(programs[1], "./drone");
    strcpy(programs[2], "./input");
    strcpy(programs[3], "./WD");

    // Pids for all children
    pid_t child[num_children];

    // string to contain all che children pids (except WD)
    char child_pids_str[num_children - 1][80];


    for (int i = 0; i < num_children; i++) {
        child[i] = Fork();
        if (!child[i]) {
            // spawn the first program, which is the server
            if (i == 0) {
                char *arg_list[] = {programs[i], NULL};
                spawn(arg_list);
            }

            // spawn the drone and the input programs
            if (i == 2) {
                char *tmp[] = {"konsole", "-e", programs[i], NULL};
                Execvp("konsole", tmp);
            }
            // spawn the drone
            if (i > 0 && i < num_children - 1) {
                char *arg_list[] = {programs[i], NULL};
                spawn(arg_list);
            }

            // spawn the last program, so the WD, which needs all the processes
            // PIDs
            if (i == num_children - 1) {
                for (int i = 0; i < num_children - 1; i++)
                    sprintf(child_pids_str[i], "%d", child[i]);

                char *arg_list[] = {programs[i], child_pids_str[0],
                                    child_pids_str[1], child_pids_str[2], NULL};
                spawn(arg_list);
            }
        }
    }

    printf("Server pid is %d\n", child[0]);
    printf("Drone pid is %d\n", child[1]);
    printf("Konsole of Input pid is %d\n", child[2]);
    printf("WD pid is %d\n", child[3]);

    // value for waiting for the children to terminate
    int res;

    // wait for all children to terminate
    for (int i = 0; i < num_children; i++) {
        int ret = Wait(&res);
        int status;
        WEXITSTATUS(status);
        printf("Process %d terminated with code: %d\n", ret, status);
    }

    return 0;
}
