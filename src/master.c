#include "constants.h"
#include "wrapFuncs/wrapFunc.h"
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

// Function to spawn the processes
static void spawn(char **arg_list) {
    Execvp(arg_list[0], arg_list);
    exit(EXIT_FAILURE);
}

int main(int argc, char *argv[]) {
    // Specifying that argc and argv are unused variables
    (void)(argc);
    (void)(argv);

    // Define the number of programs to spawn
    int num_children = NUM_PROCESSES;

    // Define an array of strings for every process to spawn
    char programs[num_children][20];
    strcpy(programs[0], "./server");
    strcpy(programs[1], "./drone");
    strcpy(programs[2], "./input");
    strcpy(programs[3], "./WD");

    // Pids for all children
    pid_t child[num_children];

    // String to contain all che children pids (except WD)
    char child_pids_str[num_children - 1][80];

    // Cycle neeeded to fork the correct number of childs
    for (int i = 0; i < num_children; i++) {
        child[i] = Fork();
        if (!child[i]) {
            // Spawn the first process, which is the server
            if (i == 0) {
                char *arg_list[] = {programs[i], NULL};
                spawn(arg_list);
            }

            // Spawn the input process using konsole
            if (i == 2) {
                char *tmp[] = {"konsole", "-e", programs[i], NULL};
                Execvp("konsole", tmp);
            }
            // Spawn the drone process
            if (i > 0 && i < num_children - 1) {
                char *arg_list[] = {programs[i], NULL};
                spawn(arg_list);
            }

            // spawn the last program, so the WD, which needs all the processes
            // PIDs
            if (i == num_children - 1) {
                for (int i = 0; i < num_children - 1; i++)
                    sprintf(child_pids_str[i], "%d", child[i]);

                // Sending as arguments to the WD all the processes PIDs
                char *arg_list[] = {programs[i], child_pids_str[0],
                                    child_pids_str[1], child_pids_str[2], NULL};
                spawn(arg_list);
            }
        }
    }

    // Printting the pids
    printf("Server pid is %d\n", child[0]);
    printf("Drone pid is %d\n", child[1]);
    printf("Konsole of Input pid is %d\n", child[2]);
    printf("WD pid is %d\n", child[3]);

    // Value for waiting for the children to terminate
    int res;

    // Wait for all children to terminate
    for (int i = 0; i < num_children; i++) {
        int ret = Wait(&res);
        // Getting the exit status
        int status = 0;
        WEXITSTATUS(status);
        printf("Process %d terminated with code: %d\n", ret, status);
    }

    return EXIT_SUCCESS;
}
