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

    // pipe for the communication between input and drone
    int ItoD[2];
    Pipe(ItoD);
    // value for waiting for the children to terminate
    int res;

    /*
    Pipe are not needed here since we use shared memory for the server
    // create the pipe and print the ids in a string
    Pipe(ItoD);
    char ItoD_string[80];
    sprintf(ItoD_string, "%d %d", ItoD[0], ItoD[1]);
    */

    for (int i = 0; i < num_children; i++) {
        child[i] = Fork();
        if (!child[i]) {
            // spawn the first program, which is the server
            if (i == 0) {
                char *arg_list[] = {programs[i], NULL};
                spawn(arg_list);
            }

            // spawn the drone and the input programs
            // TODO maybe
            char aux1[10];
            char aux2[10];
            sprintf(aux1, "%d", ItoD[0]);
            sprintf(aux2, "%d", ItoD[1]);
            if (i == 2) {
                char *tmp[] = {"konsole", "-e", programs[i], aux1, aux2, NULL};
                Execvp("konsole", tmp);
            }
            // TODO change back -2 to -1
            //spawn the drone
            if (i > 0 && i < num_children - 1) {
                char *arg_list[] = {programs[i],aux1, aux2, NULL};
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
        } else {
            //printf("Spawned child with pid %d\n", child[i]);
        }
    }

    printf("Server pid is %d\n", child[0]);
    printf("Drone pid is %d\n", child[1]);
    printf("Konsole of Input pid is %d\n", child[2]);
    printf("WD pid is %d\n", child[3]);
    // wait for all children to terminate
    for (int i = 0; i < num_children; i++) {
        int ret = Wait(&res);
        int status;
        WEXITSTATUS(status);
        printf("Process %d terminated with code: %d\n", ret, status);
    }

    return 0;
}
