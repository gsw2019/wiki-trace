/**
 * function prototypes for the ncurses user interface of wiki-trace
 *
 */

#include <ncurses.h>

#define WELCOME_HEIGHT  5
#define WELCOME_WIDTH   75

#define AUTHOR_HEIGHT 1
#define AUTHOR_WIDTH  75

#define MENU_HEIGHT   10
#define MENU_WIDTH    75
#define START_TRACE   0
#define SETTINGS      1
#define ABOUT         2
#define EXIT          3

#define ABOUT_HEIGHT  40
#define ABOUT_WIDTH   85
#define PAD_ROWS      100
#define PAD_COLS      75

// functions concerened with rendering start screen
void init_wiki_trace_view();
void show_welcome(WINDOW* title);
void show_author(WINDOW* author);
static void show_menu(WINDOW* menu);
static void update_menu(WINDOW* menu, int highlighted);

// functions concerned with rendering about screen
void show_about();
