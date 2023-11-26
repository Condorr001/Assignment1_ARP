#include "constants.h"
#include "dataStructs.h"
#include "ncurses.h"
#include "wrapFuncs/wrapFunc.h"
#include <curses.h>
#include <fcntl.h>
#include <math.h>
#include <semaphore.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <string.h>
#include <sys/select.h>
#include <unistd.h>
#include <sys/stat.h> 
#include <sys/types.h> 

// WD pid
pid_t WD_pid;

void signal_handler(int signo, siginfo_t *info, void *context) {
    if (signo == SIGUSR1) {
        WD_pid = info->si_pid;
        Kill(WD_pid, SIGUSR2);
    }
}

WINDOW *create_map_win(int height, int width, int starty, int startx) {
    WINDOW *local_win;

    local_win = newwin(height, width, starty, startx);
    box(local_win, 0, 0); /* 0, 0 gives default characters
                           * for the vertical and horizontal
                           * lines			*/
    // wrefresh(local_win);  /* Show that box 		*/

    return local_win;
}

void destroy_map_win(WINDOW *local_win) {
    /* box(local_win, ' ', ' '); : This won't produce the desired
     * result of erasing the window. It will leave it's four corners
     * and so an ugly remnant of window.
     */
    wborder(local_win, ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ');
    /* The parameters taken are
     * 1. win: the window on which to operate
     * 2. ls: character to be used for the left side of the window
     * 3. rs: character to be used for the right side of the window
     * 4. ts: character to be used for the top side of the window
     * 5. bs: character to be used for the bottom side of the window
     * 6. tl: character to be used for the top left corner of the window
     * 7. tr: character to be used for the top right corner of the window
     * 8. bl: character to be used for the bottom left corner of the window
     * 9. br: character to be used for the bottom right corner of the window
     */
    wrefresh(local_win);
    delwin(local_win);
}

int main(int argc, char *argv[]) {
    // signal setup
    struct sigaction sa;
    // memset(&sa, 0, sizeof(sa));

    sa.sa_sigaction = signal_handler;
    sigemptyset(&sa.sa_mask);
    // The restart flag is used to restart all those syscalls that can get
    // interrupted by signals
    sa.sa_flags = SA_SIGINFO | SA_RESTART;

    if (sigaction(SIGUSR1, &sa, NULL) < 0) {
        perror("SIGUSR1: sigaction()");
        exit(1);
    }
    
    //named pipe (fifo) to send the pid to the WD
    int fd; 
    char * fifo_one = "/tmp/fifo"; 
    Mkfifo(fifo_one, 0666); 

    int map_pid = getpid();
    char map_pid_str[10];
    sprintf(map_pid_str, "%d", map_pid);

    fd = Open(fifo_one, O_WRONLY);
    Write(fd, map_pid_str, strlen(map_pid_str)+1); 
    Close(fd); 

    // Setting up the struct in which to store the position of the drone
    // in order to calculate the current position on the screen of the drone
    struct pos drone1_pos;

    // initialize semaphor
    sem_t *sem_force = Sem_open(SEM_PATH_FORCE, O_CREAT, S_IRUSR | S_IWUSR, 1);
    sem_t *sem_position = Sem_open(SEM_PATH_POSITION, O_CREAT, S_IRUSR | S_IWUSR, 1);
    sem_t *sem_velocity = Sem_open(SEM_PATH_VELOCITY, O_CREAT, S_IRUSR | S_IWUSR, 1);

    // create shared memory object
    int shm = Shm_open(SHMOBJ_PATH, O_CREAT | O_RDWR, S_IRWXU | S_IRWXG);

    // truncate size of shared memory
    Ftruncate(shm, MAX_SHM_SIZE);

    // map pointer
    void *shm_ptr =
        Mmap(NULL, MAX_SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm, 0);

    // setting up ncurses
    initscr();
    // disabling line buffering
    cbreak();
    // hide the cursor in order to not see the white carret on the screen
    curs_set(0);
    // TODO setting up the menu
    mvprintw(0, 0, "menu");
    // Display the menu text
    refresh();
    // displaying the first intance of the window
    WINDOW *map_window =
        create_map_win(getmaxy(stdscr) - 2, getmaxx(stdscr), 1, 0);

    // setting up structures needed for terminal resizing
    while (1) {
        // Updating the drone position by reading from the shared memory
        Sem_wait(sem_position);
        sscanf(shm_ptr, "%f|%f", &drone1_pos.x, &drone1_pos.y);
        Sem_post(sem_position);

        // Deleting the old window that is encapsulating the map in order to
        // create the animation
        delwin(map_window);
        // Redrawing the window. This is usefull if the screen is resized
        map_window = create_map_win(LINES - 1, COLS, 1, 0);
        // In order to correctly handle the drone position calculation all
        // the simulations are done in a 500x500 square. Then the position of
        // the drone is converted to the dimension of the window by doing a
        // proportion. The computation done in the following line could by
        // expressed as the following in the x axis:
        // drone_position_in_terminal = simulated_drone_pos_x * term_width/500
        // Now for the terminal width and height it needs to be taken into
        // consideration the border of the window itself. Considering for
        // example the x axis we have a border on the left and one on the right
        // so we need to subtract 2 from the width of the main_window. Now the
        // extra -1 is due to the fact that the index starts from 0 and not
        // from 1. With this we mean that if we have an array of dimension 3.
        // The highest index of an element in the arry will be 2, not 3. This
        // explains why -3 instead of -2.
        int x = round(1 + drone1_pos.x * (getmaxx(map_window) - 3) /
                              SIMULATION_WIDTH);
        int y = round(1 + drone1_pos.y * (getmaxy(map_window) - 3) /
                              SIMULATION_HEIGHT);

        // The drone is now displayed on the screen
        mvwprintw(map_window, y, x, "+");

        // The map_window is refreshed
        wrefresh(map_window);
        // The process sleeps for the time needed to have an almost 30fps
        // animation
        usleep(3000);
    }

    // Clean up
    shm_unlink(SHMOBJ_PATH);
    Sem_close(sem_force);
    Sem_close(sem_velocity);
    Sem_close(sem_position);
    Sem_unlink(SEM_PATH_POSITION);
    Sem_unlink(SEM_PATH_FORCE);
    Sem_unlink(SEM_PATH_VELOCITY);
    munmap(shm_ptr, MAX_SHM_SIZE);
    // Closing ncurses
    endwin();
    return EXIT_SUCCESS;
}
