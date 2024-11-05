#ifndef UI_H
#define UI_H
#include <curses.h>
struct UI {
    WINDOW *root;
    int rows, cols;

    WINDOW *create_window(int height, int width, int starty, int startx, const char* label) {
        WINDOW* win;
        WINDOW* actual_window;

        win = newwin(height, width, starty, startx);
        box(win, 0 , 0);

        // add inner borders for label
        mvwaddch(win, 2, 0, ACS_LTEE);
        mvwhline(win, 2, 1, ACS_HLINE, width - 2);
        mvwaddch(win, 2, width - 1, ACS_RTEE);

        // print label in the middle
        mvwprintw(win, 1, (width - strlen(label)) / 2 - startx, "%s", label);

        wrefresh(win);

        actual_window = newwin(height - 4, width - 2, starty + 3, startx + 1);
        return actual_window;
    }

    UI(const char* label) {
        initscr();
        cbreak();
        keypad(stdscr, TRUE);

        getmaxyx(stdscr, rows, cols);
    
        root = create_window(rows, cols, 0, 0, label);
        scrollok(root, TRUE);
    }

    ~UI() {
        endwin();
    }

    print(const char *message, ...) {
        va_list args;

        wscrl(root, 1);
        wmove(root, rows - 5, 0);

        va_start(args, message);
        vwprintw(root, message, args);
        va_end(args);

        wrefresh(root);
    }

    printchars(const char *message, ...) {
        va_list args;

        va_start(args, message);
        vwprintw(root, message, args);
        va_end(args);

        wrefresh(root); 
    }
};
#endif
