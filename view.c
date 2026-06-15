/**
 * implementations of view.h prototypes
 *
 */


#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "view.h"


// menu details
char* choices[] = {   // menu choices
  "Start a trace              Provide a starting and destination wiki page",
  "Settings                                                Tune the tracer",
  "About                     Learn about the game and how the tracer works",
  "Exit                                                                   "
};
int n_choices = sizeof(choices) / sizeof(char*);

TracePages pages;   // store pages provided by user
TraceHistory history;  // accessible window that trace history is written to

InitWindows init_windows;   // launch screen windows
TraceWindows trace_windows;   // trace screen windows
AboutWindows about_windows;   // about screen windows

/*
 * Initialize ncurses stdscr window with a welcome, menu, and author acknowledgment.
 */
void init_wiki_trace_view() {
  clear();
  refresh();

  initscr();              // begin curses mode

  // ensure terminal window size is of minimal compatible dimensions (85x40)
  if (LINES < 40 || COLS < 85) {
    endwin();
    fprintf(stderr, "Error: terminal window must be at least 85x40 to render wiki-trace\n");
    exit(1);
  }

  cbreak();               // allow program interupt signals
  noecho();               // dont show keystrokes pressed by user
  start_color();          // enable colors
  use_default_colors();   // keeps user default terminal color scheme or theme
  curs_set(0);            // remove cursor so not dangling after last write
  
  // get center of stdsc for menu
  int menu_start_y = (LINES - MENU_WIN_HEIGHT) / 2;
  int menu_start_x = (COLS - MENU_WIN_WIDTH) / 2;
  init_windows.menu_window = newwin(MENU_WIN_HEIGHT, MENU_WIN_WIDTH, menu_start_y, menu_start_x); 

  // welcome window above center
  int welcome_start_y = menu_start_y - WELCOME_WIN_HEIGHT - 2;
  int welcome_start_x = (COLS - WELCOME_WIN_WIDTH) / 2;
  init_windows.welcome_window = newwin(WELCOME_WIN_HEIGHT, WELCOME_WIN_WIDTH, welcome_start_y, welcome_start_x);

  // author window at bottom of stdscr
  int author_start_y = LINES - AUTHOR_WIN_HEIGHT - 1;
  int author_start_x = (COLS - AUTHOR_WIN_WIDTH) / 2;
  init_windows.author_window = newwin(AUTHOR_WIN_HEIGHT, AUTHOR_WIN_WIDTH, author_start_y, author_start_x);
  
  show_welcome();
  show_author();
  show_menu();   // infinite loop so must be last
}


/*
 * Show window for welcome message.
 *
 * @param welcome_window: window where message will be written
 */
static void show_welcome() {
  box(init_windows.welcome_window, 0, 0);

  char* welcome_message = "Welcome To The Wikipedia Trace Solver!";

  // print welcome message in the top middle of menu
  wattron(init_windows.welcome_window, A_BOLD | A_UNDERLINE);
  mvwprintw(init_windows.welcome_window, 2, WELCOME_WIN_WIDTH/2 - strlen(welcome_message)/2, welcome_message);
  wattroff(init_windows.welcome_window, A_BOLD | A_UNDERLINE);

  wrefresh(init_windows.welcome_window);
}


/*
 * Show a window for author.
 *
 * @param author_window: window where author info will be written
 */
static void show_author() {
  char* author_tag = "(c) Garret Wilson 2026";

  init_pair(1, COLOR_RED, COLOR_BLUE);
  wbkgd(init_windows.author_window, COLOR_PAIR(1));

  wattron(init_windows.author_window, A_BOLD | A_ITALIC); 
  mvwprintw(init_windows.author_window, 0, AUTHOR_WIN_WIDTH/2 - strlen(author_tag)/2, author_tag);
  wattroff(init_windows.author_window, A_BOLD | A_ITALIC);

  wrefresh(init_windows.author_window);
}


/*
 * Displays a welcome message and menu that allows the user to make a selection
 * between starting the game, adjusting some settings, showing an about page, or exiting.
 *
 * @param menu_window: window which will hold menu options and short descriptions
 */
static void show_menu() {
  int highlighted = 1;    // need var for tracking highlighted
  int choice = 0;   // and for when a selection is made

  keypad(init_windows.menu_window, TRUE);    // need arrow presses in menu window

  update_menu(highlighted);    // draw the menu

  // determine new state of menu from key press and current selection
  while (1) {
    int c = wgetch(init_windows.menu_window);

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
				break;
    }

    update_menu(highlighted);     // redraw after updated state
    if (choice != 0) { break; }   // exit infinite loop if we updated choice
  }

  // determine course of action based on menu choice
  switch(choice - 1) {
    case OPT_START_TRACE:
      delwin(init_windows.menu_window);
      delwin(init_windows.author_window);
      delwin(init_windows.welcome_window);
      show_trace();
    case OPT_SETTINGS:
      // TODO
      return;
    case OPT_ABOUT:
      delwin(init_windows.menu_window);
      delwin(init_windows.author_window);
      delwin(init_windows.welcome_window);
      show_about();
    case OPT_EXIT:
      clear();
      refresh();
      endwin();
  }
}


/*
 * Redraw the menu after a new key is pressed to move the in-selection cursor.
 *
 * @param menu_window: the window that will be rerendered
 * @param highlighted: value that desscribes which selection the user is hovering on
 */
static void update_menu(int highlighted) {
  box(init_windows.menu_window, 0, 0);

  int x = 2;
  int y = 3;

  // draw each menu option
  for (int i = 0; i < n_choices; i++) {
    // highlight the ooption the cursor is on
    if (highlighted == i + 1) {   // highlighted is 1 indexed
      wattron(init_windows.menu_window, A_REVERSE);
      mvwprintw(init_windows.menu_window, y, x, "%s", choices[i]);
      wattroff(init_windows.menu_window, A_REVERSE);
    } 
    else {
      mvwprintw(init_windows.menu_window, y, x, choices[i]);
    }

    y++;    // move down for each new choice
  }

  wrefresh(init_windows.menu_window);
}


/*
 * Display screen where user will input start page (spage) and destination page (dpage) then initate 
 * the trace. Will also display a live tracing of the pages walked
 */
static void show_trace() {
  // clear stdscr
  clear();
  refresh();

  // build trace window
  int trace_window_start_y = (LINES - TRACE_WIN_HEIGHT) / 2;
  int trace_window_start_x = (COLS - TRACE_WIN_WIDTH) / 2;
  trace_windows.main_window = newwin(TRACE_WIN_HEIGHT, TRACE_WIN_WIDTH, trace_window_start_y, trace_window_start_x);
  box(trace_windows.main_window, 0, 0);

  // window title
  char* trace_window_title = "Run A Trace";
  wattron(trace_windows.main_window, A_BOLD);
  mvwprintw(trace_windows.main_window, 0, TRACE_WIN_WIDTH/2 - strlen(trace_window_title)/2, trace_window_title);
  wattroff(trace_windows.main_window, A_BOLD);

  char* header_message1 = "Walk one of the possible paths that exists between the start Wikipedia page and";
  char* header_message2 = "the destination Wikipedia page. Pages that are extremely different will take";
  char* header_message3 = "longer to search for a path, if one exists.";
  mvwprintw(trace_windows.main_window, 1, TRACE_WIN_WIDTH/2 - strlen(header_message1)/2, header_message1);
  mvwprintw(trace_windows.main_window, 2, TRACE_WIN_WIDTH/2 - strlen(header_message2)/2, header_message2);
  mvwprintw(trace_windows.main_window, 3, TRACE_WIN_WIDTH/2 - strlen(header_message3)/2, header_message3);

  // arrow pointing from start page to destination page
  char* arrow = "======>";
  wattron(trace_windows.main_window, A_BOLD);
  mvwprintw(trace_windows.main_window, 6, TRACE_WIN_WIDTH/2 - strlen(arrow)/2, arrow);
  wattroff(trace_windows.main_window, A_BOLD);

  // inner window for start page (spage)
  int spage_window_y = trace_window_start_y + 5;
  int spage_window_x = trace_window_start_x + 2;
  trace_windows.spage_window = newwin(TEXT_WIN_HEIGHT, TEXT_WIN_WIDTH, spage_window_y, spage_window_x);
  box(trace_windows.spage_window, 0, 0);
  char* spage_window_title = "start page";
  mvwprintw(trace_windows.spage_window, 0, 2, spage_window_title);

  // text field for spage window
  int spage_text_field_y = spage_window_y + 1;
  int spage_text_field_x = spage_window_x + 1;
  trace_windows.spage_text_field = newpad(TEXT_FIELD_HEIGHT, TEXT_FIELD_WIDTH);
  int spage_min_row = 0;
  int spage_min_col = 0;
  int spage_view_top  = spage_window_y + 1;
  int spage_view_left = spage_window_x + 1;
  int spage_view_bot  = spage_window_y + 1;
  int spage_view_right = spage_window_x + TEXT_WIN_WIDTH - 2;
  int spage_view_height = spage_view_bot - spage_view_top + 1;

  // inner window for destination page (dpage)
  int dpage_window_y = trace_window_start_y + 5;
  int dpage_window_x = trace_window_start_x + TRACE_WIN_WIDTH - TEXT_WIN_WIDTH - 2;
  trace_windows.dpage_window = newwin(TEXT_WIN_HEIGHT, TEXT_WIN_WIDTH, dpage_window_y, dpage_window_x);
  box(trace_windows.dpage_window, 0, 0);
  char* dpage_window_title = "destination page";
  mvwprintw(trace_windows.dpage_window, 0, 2, dpage_window_title);

  // text field for dpage window
  trace_windows.dpage_text_field = newpad(TEXT_FIELD_HEIGHT, TEXT_FIELD_WIDTH);
  int dpage_min_row = 0;
  int dpage_min_col = 0;
  int dpage_view_top  = dpage_window_y + 1;
  int dpage_view_left = dpage_window_x + 1;
  int dpage_view_bot  = dpage_window_y + 1;
  int dpage_view_right = dpage_window_x + TEXT_WIN_WIDTH - 2;

  // inner window for trace history 
  int hist_start_y = trace_window_start_y + 9;
  int hist_start_x = trace_window_start_x + (TRACE_WIN_WIDTH - HIST_WIN_WIDTH) / 2;
  WINDOW* hist_window = newwin(HIST_WIN_HEIGHT, HIST_WIN_WIDTH, hist_start_y, hist_start_x);
  box(hist_window, 0, 0);
  char* hist_window_title = "trace history";
  mvwprintw(hist_window, 0, 2, hist_window_title);

  // text field for trace history
  history.text_field = newpad(HIST_TEXT_FIELD_HEIGHT, HIST_TEXT_FIELD_WIDTH);
  history.min_row = 0;
  history.min_col = 0;
  history.view_top = hist_start_y + 1;
  history.view_left = hist_start_x + 2;
  history.view_bot = hist_start_y + HIST_WIN_HEIGHT - 2;
  history.view_right = hist_start_x + HIST_WIN_WIDTH - 2;

  for (int i = 0; i < HIST_TEXT_FIELD_HEIGHT; i++) {
    mvwprintw(history.text_field, i, 1, "this is entry #%d", i);
  }

  // actions at the bottom
  char* start = "(s) start";
  char* pause = "(p) pause";
  char* resume = "(r) resume";
  char* stop = "(x) stop";
  char* change_spage = "(f) start page";
  char* change_dpage = "(t) dest page";
  char* back = "(b) back";
  char* quit = "(q) quit";

  int col_width = TRACE_WIN_WIDTH / 4;
  int row1 = TRACE_WIN_HEIGHT - 5;
  int row2 = TRACE_WIN_HEIGHT - 3;

  mvwprintw(trace_windows.main_window, row1, col_width*0 + col_width/2 - strlen(start)/2, start);
  mvwprintw(trace_windows.main_window, row1, col_width*1 + col_width/2 - strlen(pause)/2, pause);
  mvwprintw(trace_windows.main_window, row1, col_width*2 + col_width/2 - strlen(resume)/2, resume);
  mvwprintw(trace_windows.main_window, row1, col_width*3 + col_width/2 - strlen(stop)/2, stop);


  mvwprintw(trace_windows.main_window, row2, col_width*0 + col_width/2 - strlen(change_spage)/2, change_spage);
  mvwprintw(trace_windows.main_window, row2, col_width*1 + col_width/2 - strlen(change_dpage)/2, change_dpage);
  mvwprintw(trace_windows.main_window, row2, col_width*2 + col_width/2 - strlen(back)/2, back);
  mvwprintw(trace_windows.main_window, row2, col_width*3 + col_width/2 - strlen(quit)/2, quit);

  // render the trace screen
  wrefresh(trace_windows.main_window);
  wrefresh(trace_windows.spage_window);
  wrefresh(trace_windows.dpage_window);
  wrefresh(trace_windows.hist_window);
  prefresh(trace_windows.spage_text_field, spage_min_row, spage_min_col, spage_view_top, spage_view_left, spage_view_bot, spage_view_right);
  prefresh(trace_windows.dpage_text_field, dpage_min_row, dpage_min_col, dpage_view_top, dpage_view_left, dpage_view_bot, dpage_view_right);
  prefresh(history.text_field, history.min_row, history.min_col, history.view_top, history.view_left, history.view_bot, history.view_right);

  // handle actions via key presses
  while (1) {
    int ch = wgetch(trace_windows.main_window);

    switch(ch) {
      case 's':
      case 'S':
        // TODO
        break;
      case 'p':
      case 'P':
        // TODO
        break;
      case 'r':
      case 'R':
        // TODO
        break;
      case 'x':
      case 'X':
        // TODO
        break;
      case 'f':
      case 'F':
        read_user_input(trace_windows.spage_text_field, spage_min_row, spage_min_col, spage_view_top, spage_view_left, spage_view_bot, spage_view_right, strlen(pages.start_page));
        break;
      case 't':
      case 'T':
        read_user_input(trace_windows.dpage_text_field, dpage_min_row, dpage_min_col, dpage_view_top, dpage_view_left, dpage_view_bot, dpage_view_right, strlen(pages.dest_page));
        break;
      case 'b':
      case 'B':
        delwin(trace_windows.main_window);
        delwin(trace_windows.spage_window);
        delwin(trace_windows.spage_text_field);
        delwin(trace_windows.dpage_window);
        delwin(trace_windows.dpage_text_field);
        delwin(trace_windows.hist_window);
        delwin(history.text_field);
        init_wiki_trace_view();
      case 'q':
      case 'Q':
        delwin(trace_windows.main_window);
        delwin(trace_windows.spage_window);
        delwin(trace_windows.spage_text_field);
        delwin(trace_windows.dpage_window);
        delwin(trace_windows.dpage_text_field);
        delwin(trace_windows.hist_window);
        delwin(history.text_field);
        endwin();
        return;
    }
  }
}


/*
 * as the trace runs, call this function which will write updates into the history
 */
void update_trace_history() {

}


/*
 * Read chars entered into the start page text field or the destination page text field.
 * Also updates the text field on key press.
 *
 * @param text_field: the window to display and read text from
 * @param min_row: the starting row displayed in the window
 * @param min_col: the staring column displyed in the window
 * @param view_top: location of the top of the window
 * @param view_left: location of the left of the window
 * @param view_bot: location of the bottom of the window
 * @param view_right: location of the right of the window
 * @param index: the current index of the cursor and buffer
 */
static void read_user_input(WINDOW* text_field, int min_row, int min_col, int view_top, int view_left, int view_bot, int view_right, int index) {
  curs_set(1);
  wmove(text_field, 0, index);
  prefresh(text_field, 0, 0, view_top, view_left, view_bot, view_right);
  keypad(text_field, TRUE);
  char page[256];

  while(1) {
    int ch = wgetch(text_field);
    // ASCCII chars
    if (ch >= 32 && ch <= 127 && index <= 255) {
      waddch(text_field, ch);   // add to text field
      page[index] = ch;    // store it
      index++;    // increment cursor and next spot to fill in buffer
      update_text_field_view(text_field, min_row, min_col, view_top, view_left, view_bot, view_right, index);
    }
    // deleting chars
    else if (ch == KEY_BACKSPACE && index >= 0) {
      // deletes the last char to completely clear text field
      if (index == 0) { 
        waddch(text_field, ' ');
        wmove(text_field, 0, index);
        prefresh(text_field, 0, 0, view_top, view_left, view_bot, view_right);
        continue;
      }
      waddch(text_field, ' ');
      page[index] = '\0';
      index--;
      update_text_field_view(text_field, min_row, min_col, view_top, view_left, view_bot, view_right, index);
    }
    // user pressed enter
    else if (ch == 10) {
      int starty, startx;
      getbegyx(text_field, starty, startx);
      if (startx < COLS / 2) { strcpy(pages.start_page, page); }
      else { strcpy(pages.dest_page, page); }
      curs_set(0);
      return;
    }
  }
}


/*
 * updates the text field view when user types
 *
 * @param text_field: the window to display and read text from
 * @param min_row: the starting row displayed in the window
 * @param min_col: the staring column displyed in the window
 * @param view_top: location of the top of the window
 * @param view_left: location of the left of the window
 * @param view_bot: location of the bottom of the window
 * @param view_right: location of the right of the window
 * @param index: the current index of the cursor and buffer
 */
static void update_text_field_view(WINDOW* text_field, int min_row, int min_col, int view_top, int view_left, int view_bot, int view_right, int index) {
  int new_min_col;

  if (index >= TEXT_WIN_WIDTH - 2) {
    new_min_col = index - TEXT_WIN_WIDTH + 2;
  }
  else {
    new_min_col = 0;
  }

  wmove(text_field, 0, index);
  prefresh(text_field, min_row, new_min_col, view_top, view_left, view_bot, view_right);
}


/*
 * getter for trace pages
 *
 * @return pages: a struct containing start page and destination page
 */
TracePages get_trace_pages() { return pages; }


/*
 * getter for history text field window
 *
 * @retun history: struct with all necessary fields to write to and update a text field window
 */
TraceHistory get_trace_history() { return history; }


/*
 * Display screen that allows user to adjust some settings of the tracer
 */
static void show_settings() {
  // TODO
}


/*
 * Display the about screen describing the game and how the tracer works.
 */
static void show_about() {
  // clear the stdscr so can show new display 
  clear();
  refresh();

  // draw about window
  int about_start_y = (LINES - ABOUT_WIN_HEIGHT) / 2;
  int about_start_x = (COLS - ABOUT_WIN_WIDTH) / 2;
  about_windows.main_window = newwin(ABOUT_WIN_HEIGHT, ABOUT_WIN_WIDTH, about_start_y, about_start_x);
  keypad(about_windows.main_window, TRUE);
  box(about_windows.main_window, 0, 0);

  // title of window
  char* window_title = "ABOUT";
  wattron(about_windows.main_window, A_BOLD);
  mvwprintw(about_windows.main_window, 0, ABOUT_WIN_WIDTH/2 - strlen(window_title)/2, window_title);
  wattroff(about_windows.main_window, A_BOLD);

  // description of window
  mvwprintw(about_windows.main_window, 1, 3, "Share details about the Wikipedia trace game and how this program works to find");
  mvwprintw(about_windows.main_window, 2, 3, "potential paths between a starting page to a destination page");

  // actions at the bottom
  char* back = "(b) back to menu";
  char* quit = "(q) quit";
  mvwprintw(about_windows.main_window, ABOUT_WIN_HEIGHT - 3, ABOUT_WIN_WIDTH/4 - strlen(back)/2, back);
  mvwprintw(about_windows.main_window, ABOUT_WIN_HEIGHT - 3, ABOUT_WIN_WIDTH/2 + ABOUT_WIN_WIDTH/4 - strlen(quit)/2, quit);

  // creating a scrollable interior
  about_windows.text_field = newpad(ABOUT_PAD_ROWS, ABOUT_PAD_COLS);

  // write aactual info to the scrollable area
  // TODO
  for (int i = 0; i < ABOUT_PAD_ROWS; i++) {
    mvwprintw(about_windows.text_field, i, 1, "here is a temp entery inside this scrollaable area that I want #%d", i + 1);
  }

  // bounds for the scrollable window
  int pad_scroll_line = 0;
  int view_top  = about_start_y + 4;
  int view_left = about_start_x + 4;
  int view_bot  = about_start_y + ABOUT_WIN_HEIGHT - 6;
  int view_right = about_start_x + ABOUT_WIN_WIDTH;
  int view_height = view_bot - view_top + 1;

  // once outside loop so renders without extra key press
  wrefresh(about_windows.main_window);
  prefresh(about_windows.text_field, 
           pad_scroll_line, 0,
           view_top, view_left,
           view_bot, view_right);

  // scrollable view of information
  while (1) {
    wrefresh(about_windows.main_window);
    prefresh(about_windows.text_field,
             pad_scroll_line, 0,
             view_top, view_left,
             view_bot, view_right);

    int ch = wgetch(about_windows.main_window);
    switch(ch) {
      case KEY_UP:
        if (pad_scroll_line > 0) { pad_scroll_line--; }
        break;
      case KEY_DOWN:
        if (pad_scroll_line < (ABOUT_PAD_ROWS - view_height)) { pad_scroll_line++; }
        break;
      case 'b':
      case 'B':
        delwin(about_windows.main_window);
        delwin(about_windows.text_field);
        init_wiki_trace_view();
        return;
      case 'q':
      case 'Q':
        delwin(about_windows.main_window);
        delwin(about_windows.text_field);
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

