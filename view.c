/**
 * implementations of view.h prototypes
 */


#include "view.h"
#include <string.h>


int menu_choice;
char* choices[] = {   // menu choices
  "Start trace",
  "Settings",
  "About",
  "Exit"
};
int n_choices = sizeof(choices) / sizeof(char*);


/*
 * initialize ncurses stdscr window with a menu
 */
void init_ncurses() {
  initscr();    // begin curses mode
  cbreak();     // allow program interupt signals
  noecho();     // dont show keystrokes pressed by user
 
  // get center of stdscr
  int menu_start_y = (LINES - MENU_HEIGHT) / 2;
  int menu_start_x = (COLS - MENU_WIDTH) / 2;
  WINDOW* menu_window = newwin(MENU_HEIGHT, MENU_WIDTH, menu_start_y, menu_start_x); 

  show_menu(menu_window);
}


/*
 * function that displays a welcome message and menu that allows the user to make a selection
 * between starting the game, adjusting some settings, showing an about page, or exiting
 * 
 */
static void show_menu(WINDOW* menu_window) {
  int highlighted = 1;
  int choice = 0;

  keypad(menu_window, TRUE);    // need arrow presses in menu window

  update_menu(menu_window, highlighted);    // draw the menu

  // redraw menu depending on key press and current selection
  while (1) {
    int c = wgetch(menu_window);

    switch(c) {
      case KEY_UP:
				if(highlighted == 1)
					highlighted = n_choices;
				else
					--highlighted;
				break;

			case KEY_DOWN:
				if(highlighted == n_choices)
					highlighted = 1;
				else 
					++highlighted;
				break;

      // selection chosen, user pressed <enter>
			case 10:
				choice = highlighted;
				break;

			default:
				refresh();
				break;
    }

    update_menu(menu_window, highlighted);

    if (choice != 0) { break; }   // exit infinite loop
  }

  menu_choice = choice - 1;
}


/*
 * update the menu after a new key is pressed to move the in selection cursor
 *
 */
static void update_menu(WINDOW *menu_window, int highlighted) {
  box(menu_window, 0, 0);

  char* welcome_message = "Welcome to the wiki-trace game solver!";

  // print welcome message in the middle of menu
  wattron(menu_window, A_BOLD | A_UNDERLINE);
  mvwprintw(menu_window, 1, MENU_WIDTH/2 - strlen(welcome_message)/2, welcome_message);
  wattroff(menu_window, A_BOLD | A_UNDERLINE);

  int x = 2;
  int y = 3;

  // draw each menu option
  for (int i = 0; i < n_choices; i++) {
    // highlight the one the cursor is on
    if (highlighted == i + 1) {
      wattron(menu_window, A_REVERSE);
      mvwprintw(menu_window, y, x, "%s", choices[i]);
      wattroff(menu_window, A_REVERSE);
    } else {
      mvwprintw(menu_window, y, x, choices[i]);
    }

    y++;    // move down for each new choice
  }

  wrefresh(menu_window);

}


int get_menu_choice() {
  return menu_choice;
}

