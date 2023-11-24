#include "constants.h"
#include "dataStructs.h"
#include "utils/utils.h"
#include "wrapFuncs/wrapFunc.h"
#include <curses.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

// WD pid
pid_t WD_pid;

void signal_handler(int signo, siginfo_t *info, void *context) {
    if (signo == SIGUSR1) {
        sleep(2);
        WD_pid = info->si_pid;
        kill(WD_pid, SIGUSR2);
    }
}

WINDOW *input_display_setup(int height, int width, int starty, int startx) {
    WINDOW *local_win;

    local_win = newwin(height, width, starty, startx);
    box(local_win, 0, 0); /* 0, 0 gives default characters
                           * for the vertical and horizontal
                           * lines			*/
    // wrefresh(local_win);  /* Show that box 		*/

    return local_win;
}

void destroy_input_display(WINDOW *local_win) {
    wborder(local_win, ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ');
    wrefresh(local_win);
    delwin(local_win);
}

void begin_format_input(int input, WINDOW *tl_win, WINDOW *tc_win,
                        WINDOW *tr_win, WINDOW *cl_win, WINDOW *cc_win,
                        WINDOW *cr_win, WINDOW *bl_win, WINDOW *bc_win,
                        WINDOW *br_win) {

    // In this function the color of the arrows is enabled if the press of
    // the corresponding key in the keyboard is detected
    switch (input) {
    case 'q':
        wattron(tl_win, COLOR_PAIR(1));
        break;
    case 'w':
        wattron(tc_win, COLOR_PAIR(1));
        break;
    case 'e':
        wattron(tr_win, COLOR_PAIR(1));
        break;
    case 'a':
        wattron(cl_win, COLOR_PAIR(1));
        break;
    case 's':
        wattron(cc_win, COLOR_PAIR(1));
        break;
    case 'd':
        wattron(cr_win, COLOR_PAIR(1));
        break;
    case 'z':
        wattron(bl_win, COLOR_PAIR(1));
        break;
    case 'x':
        wattron(bc_win, COLOR_PAIR(1));
        break;
    case 'c':
        wattron(br_win, COLOR_PAIR(1));
        break;
    case ' ':
        wattron(cc_win, COLOR_PAIR(1));
        break;
    }
}

void end_format_input(int input, WINDOW *tl_win, WINDOW *tc_win, WINDOW *tr_win,
                      WINDOW *cl_win, WINDOW *cc_win, WINDOW *cr_win,
                      WINDOW *bl_win, WINDOW *bc_win, WINDOW *br_win) {

    // This function is the reciprocal of the one above infact it ripristinates
    // the color of the previously pressed key
    switch (input) {
    case 'q':
        wattroff(tl_win, COLOR_PAIR(1));
        break;
    case 'w':
        wattroff(tc_win, COLOR_PAIR(1));
        break;
    case 'e':
        wattroff(tr_win, COLOR_PAIR(1));
        break;
    case 'a':
        wattroff(cl_win, COLOR_PAIR(1));
        break;
    case 's':
        wattroff(cc_win, COLOR_PAIR(1));
        break;
    case 'd':
        wattroff(cr_win, COLOR_PAIR(1));
        break;
    case 'z':
        wattroff(bl_win, COLOR_PAIR(1));
        break;
    case 'x':
        wattroff(bc_win, COLOR_PAIR(1));
        break;
    case 'c':
        wattroff(br_win, COLOR_PAIR(1));
        break;
    case ' ':
        wattroff(cc_win, COLOR_PAIR(1));
        break;
    }
}

float diag(float side) {
    // Beeing the sqrt a slow operation and supposing that
    // this system would be required to be the fastest possible
    // a static value for the half of the sqrt of 2 is used instead of the
    // computation of sqrt
    float sqrt2_half = 0.7071;
    // The dimension of the diagonal of the square is returned
    return side * sqrt2_half;
}

float slow_down(float force_value, float step) {
    // If the force value is positive then we need to subtract
    // the step from it. If after the subtraction the value is negative, then
    // it is set to 0 because this is a breaking function and this means
    // that the force is no longer present.
    // The same goes for the negative value
    if (force_value > 0) {
        force_value -= step;
        if (force_value < 0)
            force_value = 0;
    } else if (force_value < 0) {
        force_value += step;
        if (force_value > 0)
            force_value = 0;
    }
    return force_value;
}

void update_force(struct force *to_update, int input, float step, void *shm_ptr,
                  sem_t *sem_force, float max_force) {
    // Note that the axis are positioned in this way
    //                     X
    //           +--------->
    //           |
    //           |
    //         Y |
    //           V

    // Depending on the pressed key the force is updated accordingly
    switch (input) {
    case 'q':
        to_update->x_component -= diag(step);
        to_update->y_component -= diag(step);
        break;
    case 'w':
        to_update->y_component -= step;
        break;
    case 'e':
        to_update->x_component += diag(step);
        to_update->y_component -= diag(step);
        break;
    case 'a':
        to_update->x_component -= step;
        break;
    case 's':
        to_update->x_component = slow_down(to_update->x_component, step);
        to_update->y_component = slow_down(to_update->y_component, step);
        break;
    case 'd':
        to_update->x_component += step;
        break;
    case 'z':
        to_update->x_component -= diag(step);
        to_update->y_component += diag(step);
        break;
    case 'x':
        to_update->y_component += step;
        break;
    case 'c':
        to_update->x_component += diag(step);
        to_update->y_component += diag(step);
        break;
    case ' ':
        to_update->x_component = slow_down(to_update->x_component, step);
        to_update->y_component = slow_down(to_update->y_component, step);
        break;
    }

    // If the force goes too big than is set as the max value that has
    // been read from the parameters file
    if (to_update->x_component > max_force)
        to_update->x_component = max_force;
    if (to_update->y_component > max_force)
        to_update->y_component = max_force;
    if (to_update->x_component < -max_force)
        to_update->x_component = -max_force;
    if (to_update->y_component < -max_force)
        to_update->y_component = -max_force;
}

int main(int argc, char *argv[]) {
    int reading_fd;
    int writing_fd;
    sscanf(argv[1], "%d", &reading_fd);
    sscanf(argv[2], "%d", &writing_fd);
    close(reading_fd);

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

    // path of the pid file
    char filename2_string[80];
    sprintf(filename2_string, "../file/pid.txt");

    // get current pid and save it in a string
    int input_pid = getpid();
    char input_pid_str[10];
    sprintf(input_pid_str, "%d", input_pid);

    // write input pid in the file
    FILE *F1;
    F1 = fopen(filename2_string, "w");
    fprintf(F1, "%s\n", input_pid_str);
    fclose(F1);

    // START OF NCURSES---------------------------------------

    // The max value that the force applied to the drone
    // for each axis is read.
    float max_force = get_param("input", "max_force");

    // Initializing data structs
    // The current force as the current velocity at the
    // beginning of the simulation are 0
    struct force drone_current_force = {0, 0};
    struct velocity drone_current_velocity = {0, 0};

    // The force_step, meaning how much force should be added
    // with each button press is read from the parameters file.
    float force_step = get_param("input", "force_step");
    struct pos drone_current_pos;

    /// initializing share memory and semaphores
    // initialize semaphor
    sem_t *sem_force = Sem_open(SEM_PATH_FORCE, O_CREAT, S_IRUSR | S_IWUSR, 1);
    sem_t *sem_position =
        Sem_open(SEM_PATH_POSITION, O_CREAT, S_IRUSR | S_IWUSR, 1);
    sem_t *sem_velocity =
        Sem_open(SEM_PATH_VELOCITY, O_CREAT, S_IRUSR | S_IWUSR, 1);
    // initialized to 0 until shared memory is instantiated

    // create shared memory object
    int shm = Shm_open(SHMOBJ_PATH, O_CREAT | O_RDWR, S_IRWXU | S_IRWXG);

    // truncate size of shared memory
    Ftruncate(shm, MAX_SHM_SIZE);

    // map pointer
    void *shm_ptr =
        Mmap(NULL, MAX_SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm, 0);

    // Initializing ncurses
    initscr();
    // Disable line buffering
    cbreak();
    // Disable echo of pressed characters in order to not have
    // the keys pressed to make the drone move appear on the screen
    noecho();
    // Hide the cursor in order to net see the carret while it's Displaying
    // the interface
    curs_set(0);
    // Enable the colors for ncurses
    start_color();
    // Use the default terminal colors. This is usefull while displaying the
    // current background color and leave it as the same as the one of the
    // current terminal
    use_default_colors();
    // The use of use_default_colors comes into play here where the color of the
    // background is set to -1 meaning that it becomes the current terminal
    // background color
    init_pair(1, COLOR_GREEN, -1);

    // The windows of the matrix visible in the left split are now initialized
    WINDOW *left_split = input_display_setup(LINES, COLS / 2 - 1, 0, 0);
    WINDOW *right_split = input_display_setup(LINES, COLS / 2 - 1, COLS / 2, 0);
    WINDOW *tl_win = input_display_setup(5, 7, LINES / 3, COLS / 6);
    WINDOW *tc_win = input_display_setup(5, 7, LINES / 3, COLS / 6 + 6);
    WINDOW *tr_win = input_display_setup(5, 7, LINES / 3, COLS / 6 + 12);
    WINDOW *cl_win = input_display_setup(5, 7, LINES / 3 + 4, COLS / 6);
    WINDOW *cc_win = input_display_setup(5, 7, LINES / 3 + 4, COLS / 6 + 6);
    WINDOW *cr_win = input_display_setup(5, 7, LINES / 3 + 4, COLS / 6 + 12);
    WINDOW *bl_win = input_display_setup(5, 7, LINES / 3 + 8, COLS / 6);
    WINDOW *bc_win = input_display_setup(5, 7, LINES / 3 + 8, COLS / 6 + 6);
    WINDOW *br_win = input_display_setup(5, 7, LINES / 3 + 8, COLS / 6 + 12);

    // The variable in which to store the input pressed by the user is
    // initialized here
    char input;

    // The value of the rate reductor for the read from the paraeters file is
    // first set here. This is a counter that is decreased for each cycle in
    // the main while loop. It is a way to reduce the number of reads from the
    // parameters file
    int reading_rate_reductor = get_param("input", "reading_rate_reductor");


    timeout(100);
    while (1) {
        // updating values to show in the interface. First the position
        // is updated
        Sem_wait(sem_position);
        sscanf(shm_ptr + SHM_OFFSET_POSITION, "%f|%f", &drone_current_pos.x,
               &drone_current_pos.y);
        Sem_post(sem_position);

        // Now also the velocity gets updated
        Sem_wait(sem_velocity);
        sscanf(shm_ptr + SHM_OFFSET_VELOCITY_COMPONENTS, "%f|%f",
               &drone_current_velocity.x_component,
               &drone_current_velocity.y_component);
        Sem_post(sem_velocity);

        // updating constants at runtime when the reading_rate_reductor goes to
        // 0
        if (!reading_rate_reductor--) {
            reading_rate_reductor = get_param("input", "reading_rate_reductor");
            force_step = get_param("input", "force_step");
            max_force = get_param("input", "max_force");
        }

        // Timeout sets the time in nanoseconds that getch should wait if no
        // input is received. This is the equivalent of having: usleep(100000)
        // non_blocking_getch()
        input = getch();
        if(input != ERR){
            Write(writing_fd, &input, 2);
        }

        // Calculate the currently acting force on the drone by sending the
        // currently pressed key
        update_force(&drone_current_force, input, force_step, shm_ptr,
                     sem_force, max_force);

        // Destroy all the windows in order to create the animation and
        // guarantee that the windows will be refreshed in case of the resizing
        // of the terminal
        destroy_input_display(tl_win);
        destroy_input_display(left_split);
        destroy_input_display(right_split);

        // Setting the initial values for the splits
        left_split = input_display_setup(LINES, COLS / 2 - 1, 0, 0);
        right_split = input_display_setup(LINES, COLS / 2 - 1, 0, COLS / 2);

        // Setting the initial values for the cells of the matrix containing
        // the arrows
        tl_win = input_display_setup(5, 7, LINES / 3, COLS / 6);
        tc_win = input_display_setup(5, 7, LINES / 3, COLS / 6 + 6);
        tr_win = input_display_setup(5, 7, LINES / 3, COLS / 6 + 12);
        cl_win = input_display_setup(5, 7, LINES / 3 + 4, COLS / 6);
        cc_win = input_display_setup(5, 7, LINES / 3 + 4, COLS / 6 + 6);
        cr_win = input_display_setup(5, 7, LINES / 3 + 4, COLS / 6 + 12);
        bl_win = input_display_setup(5, 7, LINES / 3 + 8, COLS / 6);
        bc_win = input_display_setup(5, 7, LINES / 3 + 8, COLS / 6 + 6);
        br_win = input_display_setup(5, 7, LINES / 3 + 8, COLS / 6 + 12);

        // Setting the "titles" of the splits
        mvwprintw(left_split, 0, 1, "INPUT DISPLAY");
        mvwprintw(right_split, 0, 1, "DYNAMICS DISPLAY");

        // Begin the coloring of the pressed key if any key is pressed
        begin_format_input(input, tl_win, tc_win, tr_win, cl_win, cc_win,
                           cr_win, bl_win, bc_win, br_win);

        mvwprintw(tl_win, 1, 3, "_");
        mvwprintw(tl_win, 2, 2, "'\\");

        mvwprintw(tc_win, 1, 3, "A");
        mvwprintw(tc_win, 2, 3, "|");

        mvwprintw(tr_win, 1, 3, "_");
        mvwprintw(tr_win, 2, 3, "/'");

        mvwprintw(cl_win, 2, 2, "<");
        mvwprintw(cl_win, 2, 3, "-");

        mvwprintw(cr_win, 2, 3, "-");
        mvwprintw(cr_win, 2, 4, ">");

        mvwprintw(bl_win, 2, 2, "|/");
        mvwprintw(bl_win, 3, 2, "'-");

        mvwprintw(bc_win, 2, 3, "|");
        mvwprintw(bc_win, 3, 3, "V");

        mvwprintw(br_win, 2, 3, "\\|");
        mvwprintw(br_win, 3, 3, "-'");

        mvwprintw(cc_win, 2, 3, "X");

        // Disable the color for the next iteration
        end_format_input(input, tl_win, tc_win, tr_win, cl_win, cc_win, cr_win,
                         bl_win, bc_win, br_win);

        // Setting the symbols for the corners of the cells
        mvwprintw(tc_win, 0, 0, ".");
        mvwprintw(tr_win, 0, 0, ".");
        mvwprintw(cl_win, 0, 0, "+");
        mvwprintw(cc_win, 0, 0, "+");
        mvwprintw(cr_win, 0, 0, "+");
        mvwprintw(cr_win, 0, 6, "+");
        mvwprintw(bl_win, 0, 0, "+");
        mvwprintw(bc_win, 0, 0, "+");
        mvwprintw(br_win, 0, 0, "+");
        mvwprintw(br_win, 0, 6, "+");
        mvwprintw(bc_win, 4, 0, "'");
        mvwprintw(br_win, 4, 0, "'");

        /// Right split

        // Displaying the current position fo the drone with nice formatting
        mvwprintw(right_split, LINES / 10 + 2, COLS / 10, "position {");
        mvwprintw(right_split, LINES / 10 + 3, COLS / 10, "\tx: %f",
                  drone_current_pos.x);
        mvwprintw(right_split, LINES / 10 + 4, COLS / 10, "\ty: %f",
                  drone_current_pos.y);
        mvwprintw(right_split, LINES / 10 + 5, COLS / 10, "}");

        // Displaying the current velocity of the drone with nice formatting
        mvwprintw(right_split, LINES / 10 + 7, COLS / 10, "velocity {");
        mvwprintw(right_split, LINES / 10 + 8, COLS / 10, "\tx: %f",
                  drone_current_velocity.x_component);
        mvwprintw(right_split, LINES / 10 + 9, COLS / 10, "\ty: %f",
                  drone_current_velocity.y_component);
        mvwprintw(right_split, LINES / 10 + 10, COLS / 10, "}");

        // Displaying the current force beeing applied on the drone only by the
        // user. So no border effect are taken into consideration while
        // displaying these values.
        mvwprintw(right_split, LINES / 10 + 12, COLS / 10, "force {");
        mvwprintw(right_split, LINES / 10 + 13, COLS / 10, "\tx: %f",
                  drone_current_force.x_component);
        mvwprintw(right_split, LINES / 10 + 14, COLS / 10, "\ty: %f",
                  drone_current_force.y_component);
        mvwprintw(right_split, LINES / 10 + 15, COLS / 10, "}");

        // Refreshing all the windows
        wrefresh(left_split);
        wrefresh(right_split);
        wrefresh(tl_win);
        wrefresh(tc_win);
        wrefresh(tr_win);
        wrefresh(cl_win);
        wrefresh(cc_win);
        wrefresh(cr_win);
        wrefresh(bl_win);
        wrefresh(bc_win);
        wrefresh(br_win);
    }

    // Closing ncurses
    endwin();

    // Cleaning up
    shm_unlink(SHMOBJ_PATH);
    Sem_close(sem_force);
    Sem_close(sem_velocity);
    Sem_close(sem_position);
    Sem_unlink(SEM_PATH_POSITION);
    Sem_unlink(SEM_PATH_FORCE);
    Sem_unlink(SEM_PATH_VELOCITY);
    munmap(shm_ptr, MAX_SHM_SIZE);
    close(writing_fd);
    return 0;
}
