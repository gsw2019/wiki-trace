/**
 * function prototypes for the ncurses user interface of wiki-trace
 *
 */

#include <ncurses.h>

#define MENU_HEIGHT 30
#define MENU_WIDTH  60

#define START_TRACE 0
#define SETTINGS    1
#define ABOUT       2
#define EXIT        3

void init_ncurses();
static void show_menu(WINDOW* menu);
static void update_menu(WINDOW* menu, int highlighted);
int get_menu_choice();


