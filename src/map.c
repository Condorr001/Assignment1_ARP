#include "constants.h"
#include "ncurses.h"
#include "wrapFuncs/wrapFunc.h"
#include <curses.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/select.h>
#include <sys/ioctl.h>
#include <unistd.h>

#define SELECT_SLEEP_TIME 100

struct drone_pos{
    float x;
    float y;
}drone1_pos = {0,0};

WINDOW *create_map_win(int height, int width, int starty, int startx) {
    WINDOW *local_win;

    local_win = newwin(height, width, starty, startx);
    box(local_win, 0, 0); /* 0, 0 gives default characters
                           * for the vertical and horizontal
                           * lines			*/
    //wrefresh(local_win);  /* Show that box 		*/

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
    // avoid error while resizing window during select sleep
    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGWINCH);
    sigprocmask(SIG_BLOCK, &mask, NULL);

    // setting up the pipe to receive data from the server
    int pipe;
    sscanf(argv[1], "%d", &pipe);

    // setting up the buffer in which to copy the data received from the server
    char received_data[100];

    // setting up the sets needed for the select
    fd_set reader;
    fd_set master;

    // setting up the timer structure that will be used by select
    struct timeval timer;
    timer.tv_sec = 0;
    timer.tv_usec = SELECT_SLEEP_TIME;

    // clearing the sets
    FD_ZERO(&reader);
    FD_ZERO(&master);

    // adding the receiving end of the pipe to the monitored file descriptors
    FD_SET(pipe, &master);
    // reading and ncurses loop

    // setting up ncurses
    initscr();
    // disabling line buffering
    cbreak();
    // hide the cursor
    curs_set(0);
    // TODO setting up the menu
    mvprintw(0, 0, "menu");
    refresh();
    // displaying the first intance of the window
    WINDOW *main_window =
        create_map_win(getmaxy(stdscr) - 2, getmaxx(stdscr), 1, 0);

    // setting up structures needed for terminal resizing
    struct winsize w;
    while (1) {
        // https://man7.org/linux/man-pages/man3/resizeterm.3x.html
        // handling terminal resizing without the use of SIGWINCH
        ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
        resizeterm(w.ws_row,w.ws_col);
        reader = master;
        // note that select with a timeaut has been used in order to guarantee
        // that ncurses will keep working even if no messages are beeing sent to
        // the map handler
        Select(pipe + 1, &reader, NULL, NULL, &timer);
        // resetting the timer after select execution
        timer.tv_usec = SELECT_SLEEP_TIME;
        if (FD_ISSET(pipe, &reader)) {
            char received[MAX_STRING_LEN];
            Read(pipe, received, MAX_STRING_LEN);

            sscanf(received, "%f|%f", &drone1_pos.x, &drone1_pos.y);
        }

        // handling ncurses update
        delwin(main_window);
        main_window =
            create_map_win(LINES - 1, COLS, 1, 0);
        int x = 1 + drone1_pos.x * (getmaxx(main_window)-3) / SIMULATION_WIDTH;
        int y = 1 + drone1_pos.y * (getmaxy(main_window)-3) / SIMULATION_HEIGHT;
        mvwprintw(main_window, y,x,"+");
        wrefresh(main_window);
    }
    endwin();
    return EXIT_SUCCESS;
}
