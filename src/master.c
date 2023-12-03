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

    // Define an array of strings for every process to spawn
    char programs[NUM_PROCESSES][20];
    strcpy(programs[0], "./server");
    strcpy(programs[1], "./drone");
    strcpy(programs[2], "./input");
    strcpy(programs[3], "./map");
    strcpy(programs[4], "./WD");

    // Pids for all children
    pid_t child[NUM_PROCESSES];

    // String to contain all che children pids (except WD)
    char child_pids_str[NUM_PROCESSES - 1][80];

    // Cycle neeeded to fork the correct number of childs
    for (int i = 0; i < NUM_PROCESSES; i++) {
        child[i] = Fork();
        if (!child[i]) {
            // Spawn the input process using konsole
            if (i == 2 || i == 3) {
                char *tmp[] = {"konsole", "-e", programs[i], NULL};
                Execvp("konsole", tmp);
            } else if (i < NUM_PROCESSES - 1) {
                char *arg_list[] = {programs[i], NULL};
                spawn(arg_list);
            }

            // spawn the last program, so the WD, which needs all the processes
            // PIDs
            if (i == NUM_PROCESSES - 1) {
                for (int i = 0; i < NUM_PROCESSES - 1; i++)
                    sprintf(child_pids_str[i], "%d", child[i]);

                // Sending as arguments to the WD all the processes PIDs
                char *arg_list[] = {programs[i],       child_pids_str[0],
                                    child_pids_str[1], child_pids_str[2],
                                    child_pids_str[3], NULL};
                spawn(arg_list);
            }
        }
    }

    // Printting the pids
    printf("Server pid is %d\n", child[0]);
    printf("Drone pid is %d\n", child[1]);
    printf("Konsole of Input pid is %d\n", child[2]);
    printf("Konsole of Map pid is %d\n", child[3]);
    printf("WD pid is %d\n", child[4]);

    // Value for waiting for the children to terminate
    int res;

    // Wait for all direct children to terminate. Map and the konsole on which
    // it runs on are not direct childs of the master process but of the server
    // one so they will not return here
    for (int i = 0; i < NUM_PROCESSES; i++) {
        int ret = Wait(&res);
        // Getting the exit status
        int status = 0;
        WEXITSTATUS(status);
        printf("Process %d terminated with code: %d\n", ret, status);
    }

    return EXIT_SUCCESS;
}
