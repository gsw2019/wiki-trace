/**
 * implementations of view.h prototypes
 *
 */


#include <string.h>

#include "view.h"


int menu_choice;
char* choices[] = {   // menu choices
  "Start a trace              Provide a starting and destination wiki page",
  "Settings                                                Tune the tracer",
  "About                     Learn about the game and how the tracer works",
  "Exit                                                                   "
};
int n_choices = sizeof(choices) / sizeof(char*);


/*
 * initialize ncurses stdscr window with a welcome, menu, and author acknowledgment
 */
void init_wiki_trace_view() {
  clear();
  refresh();

  initscr();              // begin curses mode
  cbreak();               // allow program interupt signals
  noecho();               // dont show keystrokes pressed by user
  start_color();          // enable colors
  use_default_colors();   // keeps user default terminal color scheme or theme
  curs_set(0);            // remove cursor so not dangling after last write
  
  // welcome window above center
  int welcome_start_y = (LINES - WELCOME_HEIGHT) / 3;
  int welcome_start_x = (COLS - WELCOME_WIDTH) / 2;
  WINDOW* welcome_window = newwin(WELCOME_HEIGHT, WELCOME_WIDTH, welcome_start_y, welcome_start_x);

  // get center of stdsc for menu
  int menu_start_y = (LINES - MENU_HEIGHT) / 2;
  int menu_start_x = (COLS - MENU_WIDTH) / 2;
  WINDOW* menu_window = newwin(MENU_HEIGHT, MENU_WIDTH, menu_start_y, menu_start_x); 

  // author window at bottom of stdscr
  int author_start_y = LINES - AUTHOR_HEIGHT - 1;
  int author_start_x = (COLS - AUTHOR_WIDTH) / 2;
  WINDOW* author_window = newwin(AUTHOR_HEIGHT, AUTHOR_WIDTH, author_start_y, author_start_x);
  
  show_welcome(welcome_window);
  show_author(author_window);
  show_menu(menu_window);   // infinite loop so must be last
}


/*
 * show window for welcome message
 *
 * @param title_window: window where message will be written
 */
void show_welcome(WINDOW* title_window) {
  box(title_window, 0, 0);

  char* welcome_message = "Welcome To The Wikipedia Trace Solver!";

  // print welcome message in the top middle of menu
  wattron(title_window, A_BOLD | A_UNDERLINE);
  mvwprintw(title_window, 2, WELCOME_WIDTH/2 - strlen(welcome_message)/2, welcome_message);
  wattroff(title_window, A_BOLD | A_UNDERLINE);

  wrefresh(title_window);
}


/*
 * show a window for author
 *
 * @param author_window: window where author info will be written
 */
void show_author(WINDOW* author_window) {
  char* author_tag = "(c) Garret Wilson 2026";

  init_pair(1, COLOR_MAGENTA, COLOR_BLUE);
  wbkgd(author_window, COLOR_PAIR(1));

  wattron(author_window, A_BOLD | A_ITALIC); 
  mvwprintw(author_window, 0, AUTHOR_WIDTH/2 - strlen(author_tag)/2, author_tag);
  wattroff(author_window, A_BOLD | A_ITALIC);

  wrefresh(author_window);
}


/*
 * displays a welcome message and menu that allows the user to make a selection
 * between starting the game, adjusting some settings, showing an about page, or exiting
 *
 * @param menu_window: window which will hold menu options and short descriptions
 */
static void show_menu(WINDOW* menu_window) {
  int highlighted = 1;    // need var for tracking highlighted
  int choice = 0;   // and for when a selection is made

  keypad(menu_window, TRUE);    // need arrow presses in menu window

  update_menu(menu_window, highlighted);    // draw the menu

  // determine new state of menu from key press and current selection
  while (1) {
    int c = wgetch(menu_window);

    switch(c) {
      case KEY_UP:
				if(highlighted == 1) { highlighted = n_choices; }
				else { --highlighted; }
				break;
			case KEY_DOWN:
				if(highlighted == n_choices) { highlighted = 1; }
				else { ++highlighted; }
				break;
			case 10:    // selection chosen, user pressed <enter>
				choice = highlighted;
				break;
			default:
				refresh();
				break;
    }

    update_menu(menu_window, highlighted);     // redraw after updated state
    if (choice != 0) { break; }   // exit infinite loop if we updated choice
  }

  // determine course of action based on menu choice
  switch(choice - 1) {
    case START_TRACE:
      // TODO
      return;
    case SETTINGS:
      // TODO
      return;
    case ABOUT:
      show_about();
    case EXIT:
      clrtoeol();
      refresh();
      endwin();
  }
}


/*
 * redraw the menu after a new key is pressed to move the in-selection cursor
 *
 * @param menu_window: the window that will be rerendered
 * @param highlighted: value that desscribes which selection the user is hovering on
 */
static void update_menu(WINDOW *menu_window, int highlighted) {
  box(menu_window, 0, 0);

  int x = 2;
  int y = 3;

  // draw each menu option
  for (int i = 0; i < n_choices; i++) {
    // highlight the one the cursor is on
    if (highlighted == i + 1) {   // highlighted is 1 indexed
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


/*
 * display the about screen describing the game and how the tracer works
 */
void show_about() {
  // clear the stdscr so can show new display 
  clear();
  refresh();

  // draw about window
  int about_start_y = (LINES - ABOUT_HEIGHT) / 2;
  int about_start_x = (COLS - ABOUT_WIDTH) / 2;
  WINDOW* about_window = newwin(ABOUT_HEIGHT, ABOUT_WIDTH, about_start_y, about_start_x);
  keypad(about_window, TRUE);
  box(about_window, 0, 0);

  // title of window
  wattron(about_window, A_BOLD);
  mvwprintw(about_window, 0, ABOUT_WIDTH/2 - strlen("ABOUT")/2, "ABOUT");
  wattroff(about_window, A_BOLD);

  // description of window
  mvwprintw(about_window, 1, 3, "Share details about the Wikipedia trace game and how this program works to find");
  mvwprintw(about_window, 2, 3, "potential paths between a starting page to a destination page");

  // actions at the bottom
  char* back = "(b) back to menu";
  char* quit = "(q) quit";
  mvwprintw(about_window, ABOUT_HEIGHT - 3, ABOUT_WIDTH/4 - strlen(back)/2, back);
  mvwprintw(about_window, ABOUT_HEIGHT - 3, ABOUT_WIDTH/2 + ABOUT_WIDTH/4 - strlen(quit)/2, quit);

  // creating a scrollable interior
  WINDOW* pad = newpad(PAD_ROWS, PAD_COLS);

  // write aactual info to the scrollable area
  // TODO
  for (int i = 0; i < PAD_ROWS; i++) {
    mvwprintw(pad, i, 1, "here is a temp entery inside this scrollaable area that I want #%d", i + 1);
  }

  // bounds for the scrollable window
  int pad_scroll_line = 0;
  int view_top  = about_start_y + 4;
  int view_left = about_start_x + 4;
  int view_bot  = about_start_y + ABOUT_HEIGHT - 6;
  int view_right = about_start_x + ABOUT_WIDTH;
  int view_height = view_bot - view_top + 1;

  // once outside loop so renders without extra key press
  wrefresh(about_window);
  prefresh(pad, 
           pad_scroll_line, 0,
           view_top, view_left,
           view_bot, view_right);

  // scrollable view of information
  while (1) {
    wrefresh(about_window);
    prefresh(pad,
             pad_scroll_line, 0,
             view_top, view_left,
             view_bot, view_right);

    int ch = wgetch(about_window);
    switch(ch) {
      case KEY_UP:
        if (pad_scroll_line > 0) { pad_scroll_line--; }
        break;
      case KEY_DOWN:
        if (pad_scroll_line < (PAD_ROWS - view_height)) { pad_scroll_line++; }
        break;
      case 'b':
      case 'B':
        delwin(about_window);
        delwin(pad);
        init_wiki_trace_view();
        return;
      case 'q':
      case 'Q':
        delwin(about_window);
        delwin(pad);
        endwin();
        return;
    }

  }

  /* wattron(about_window, A_BOLD); */
  /* mvwprintw(about_window, 5, 1, "THE GAME"); */
  /* wattroff(about_window, A_BOLD); */
  /* mvwprintw(about_window, 7, 6, "Wikipedia trace is blah blah blah"); */
  /**/
  /* wattron(about_window, A_BOLD); */
  /* mvwprintw(about_window, 8, 1, "THE TRACE"); */
  /* wattroff(about_window, A_BOLD); */
  /* mvwprintw(about_window, 9, 6, "This program works by blah blah blah"); */
  /**/
}

