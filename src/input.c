#include "constants.h"
#include "wrapFuncs/wrapFunc.h"
#include <curses.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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

int main(int argc, char *argv[]) {

    // signal setup
    struct sigaction sa;
    // memset(&sa, 0, sizeof(sa));

    sa.sa_sigaction = signal_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_SIGINFO;

    if (sigaction(SIGUSR1, &sa, NULL) < 0) {
        perror("SIGUSR1: sigaction()");
        exit(1);
    }

    initscr();
    cbreak();
    noecho();
    curs_set(0);
    start_color();
    use_default_colors();
    init_pair(1, COLOR_GREEN, -1);

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

    char input;
    while (1) {
        refresh();
        // sleep(1);
        //  waits half second on getch and if nothing is received goes on
        timeout(500);
        input = getch();

        destroy_input_display(tl_win);
        destroy_input_display(left_split);
        destroy_input_display(right_split);

        left_split = input_display_setup(LINES, COLS / 2 - 1, 0, 0);
        right_split = input_display_setup(LINES, COLS / 2 - 1, 0, COLS / 2);

        tl_win = input_display_setup(5, 7, LINES / 3, COLS / 6);
        tc_win = input_display_setup(5, 7, LINES / 3, COLS / 6 + 6);
        tr_win = input_display_setup(5, 7, LINES / 3, COLS / 6 + 12);
        cl_win = input_display_setup(5, 7, LINES / 3 + 4, COLS / 6);
        cc_win = input_display_setup(5, 7, LINES / 3 + 4, COLS / 6 + 6);
        cr_win = input_display_setup(5, 7, LINES / 3 + 4, COLS / 6 + 12);
        bl_win = input_display_setup(5, 7, LINES / 3 + 8, COLS / 6);
        bc_win = input_display_setup(5, 7, LINES / 3 + 8, COLS / 6 + 6);
        br_win = input_display_setup(5, 7, LINES / 3 + 8, COLS / 6 + 12);

        mvwprintw(left_split, 0, 1, "INPUT DISPLAY");
        mvwprintw(right_split, 0, 1, "DYNAMICS DISPLAY");

        begin_format_input(input, tl_win, tc_win, tr_win, cl_win, cc_win, cr_win,
                           bl_win, bc_win, br_win);

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

        end_format_input(input, tl_win, tc_win, tr_win, cl_win, cc_win, cr_win,
                         bl_win, bc_win, br_win);

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

    endwin();

    return 0;
}