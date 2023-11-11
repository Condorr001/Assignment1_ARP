#include <stdio.h>
#include <string.h> 
#include <fcntl.h> 
#include <sys/stat.h> 
#include <sys/types.h> 
#include <unistd.h> 
#include <stdlib.h>
#include <sys/wait.h>

static void spawn(char ** arg_list)
{
    execvp(arg_list[0], arg_list);
    perror("execvp()");
    exit(1);
}

void Pipe(int *fd) {
    pipe(fd);

    if(fd < 0) {
        perror("pipe()");
        exit(1);
    }
}


int main(int argc, char *argv[])
{
    //define the number of programs to spawn
    int num_children = 4;

    //define an array of strings for every process to spawn
    char programs[num_children][20];
    strcpy(programs[0], "./server");
    strcpy(programs[1], "./WD");
    strcpy(programs[2], "./drone");
    strcpy(programs[3], "./input");

    // Pids for all children
    pid_t childs[num_children];

    //pipe for the communication between input and drone
    int ItoD[2];

    //value for waiting for the children to terminate
    int res;

    //create the pipe and print the ids in a string
    Pipe(ItoD);
    char ItoD_string[80];
    sprintf(ItoD_string, "%d %d", ItoD[0], ItoD[1]);

    //spawn the first two programs, which don't need pipes
    for (int i = 0; i < num_children/2; i++) {
        char* arg_list[] =  {programs[i], NULL};
        spawn(arg_list);
    }

    //spawn the last two programs, which need pipes
    for (int i = num_children/2; i < num_children; i++) {

        char* arg_list[] = {programs[i], ItoD_string, NULL};
        spawn(arg_list);
    }
    
    //wait for all children to terminate
    for(int i = 0; i < num_children; i ++){
        wait(&res);
    }

    return 0;
}
