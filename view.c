/**
 * implementations of view.h logic
 *
 * @author Garret Wilson
 */


#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "view.h"
#include "fetcher.h"


// menu details
char* choices[] = {   // menu choices
  "Start a trace              Provide a starting and destination wiki page",
  "Settings                                                Tune the tracer",
  "About                     Learn about the game and how the tracer works",
  "Exit                                                                   "
};
int n_choices = sizeof(choices) / sizeof(char*);

pthread_t worker;   // worker thread for all none view stuff
TraceData trace_data = {   // shared data struct for view and all else
  .start_page = {0},
  .dest_page = {0},
  .currnt_page = {0},
  .status = -1,
  .lock = PTHREAD_MUTEX_INITIALIZER
};

InitWindows init_windows;   // launch screen windows
TraceWindows trace_windows;   // trace screen windows
AboutWindows about_windows;   // about screen windows


/*
 * Initialize ncurses stdscr window with a welcome, menu, and author acknowledgment.
 */
void init_view() {
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
      break;
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

    y++;    // move down for each choice
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
  WindowProps* main_window = &trace_windows.main_window;
  int trace_window_start_y = (LINES - TRACE_WIN_HEIGHT) / 2;
  int trace_window_start_x = (COLS - TRACE_WIN_WIDTH) / 2;
  main_window->window = newwin(TRACE_WIN_HEIGHT, TRACE_WIN_WIDTH, trace_window_start_y, trace_window_start_x);
  box(trace_windows.main_window.window, 0, 0);

  // window title
  main_window->title = "RUN A TRACE";
  wattron(main_window->window, A_BOLD);
  mvwprintw(main_window->window, 0, TRACE_WIN_WIDTH/2 - strlen(main_window->title)/2, main_window->title); 
  wattroff(main_window->window, A_BOLD);

  char* header_message1 = "Walk one of the possible paths that exists between the start Wikipedia page and";
  char* header_message2 = "the destination Wikipedia page. Pages that are extremely different will take";
  char* header_message3 = "longer to search for a path, if one exists.";
  mvwprintw(main_window->window, 1, TRACE_WIN_WIDTH/2 - strlen(header_message1)/2, header_message1);
  mvwprintw(main_window->window, 2, TRACE_WIN_WIDTH/2 - strlen(header_message2)/2, header_message2);
  mvwprintw(main_window->window, 3, TRACE_WIN_WIDTH/2 - strlen(header_message3)/2, header_message3);

  // arrow pointing from start page to destination page
  wattron(main_window->window, A_BOLD);
  mvwprintw(main_window->window, 6, TRACE_WIN_WIDTH/2 - strlen("======>")/2, "======>");
  wattroff(main_window->window, A_BOLD);

  // inner window for start page (spage)
  WindowProps* spage_window = &trace_windows.spage_window;
  int spage_window_y = trace_window_start_y + 5;
  int spage_window_x = trace_window_start_x + 2;
  spage_window->window = newwin(TEXT_WIN_HEIGHT, TEXT_WIN_WIDTH, spage_window_y, spage_window_x);

  spage_window->title = "start_page";
  box(spage_window->window, 0, 0);
  mvwprintw(spage_window->window, 0, 2, spage_window->title);

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
  WindowProps* dpage_window = &trace_windows.dpage_window;
  int dpage_window_y = trace_window_start_y + 5;
  int dpage_window_x = trace_window_start_x + TRACE_WIN_WIDTH - TEXT_WIN_WIDTH - 2;
  dpage_window->window = newwin(TEXT_WIN_HEIGHT, TEXT_WIN_WIDTH, dpage_window_y, dpage_window_x);

  dpage_window->title = "destination page";
  box(dpage_window->window, 0, 0);
  mvwprintw(dpage_window->window, 0, 2, dpage_window->title);

  // text field for dpage window
  trace_windows.dpage_text_field = newpad(TEXT_FIELD_HEIGHT, TEXT_FIELD_WIDTH);
  int dpage_min_row = 0;
  int dpage_min_col = 0;
  int dpage_view_top  = dpage_window_y + 1;
  int dpage_view_left = dpage_window_x + 1;
  int dpage_view_bot  = dpage_window_y + 1;
  int dpage_view_right = dpage_window_x + TEXT_WIN_WIDTH - 2;

  // color to draw active window borders
  init_pair(1, COLOR_CYAN, COLOR_BLACK);

  // inner window for trace history
  WindowProps* hist_window = &trace_windows.hist_window;
  int hist_start_y = trace_window_start_y + 9;
  int hist_start_x = trace_window_start_x + (TRACE_WIN_WIDTH - HIST_WIN_WIDTH) / 2;
  hist_window->window = newwin(HIST_WIN_HEIGHT, HIST_WIN_WIDTH, hist_start_y, hist_start_x);

  hist_window->title = "trace history";
  box(hist_window->window, 0, 0);
  mvwprintw(hist_window->window, 0, 2, hist_window->title);

  // text field for trace history
  trace_data.history.text_field = newpad(HIST_TEXT_FIELD_HEIGHT, HIST_TEXT_FIELD_WIDTH);
  trace_data.history.min_row = 0;
  trace_data.history.min_col = 0;
  trace_data.history.view_top = hist_start_y + 1;
  trace_data.history.view_left = hist_start_x + 2;
  trace_data.history.view_bot = hist_start_y + HIST_WIN_HEIGHT - 2;
  trace_data.history.view_right = hist_start_x + HIST_WIN_WIDTH - 2;
  TraceHistory history = trace_data.history;

  for (int i = 0; i < HIST_TEXT_FIELD_HEIGHT; i++) {
    mvwprintw(trace_data.history.text_field, i, 1, "this is entry #%d", i);
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

  mvwprintw(main_window->window, row1, col_width*0 + col_width/2 - strlen(start)/2, start);
  mvwprintw(main_window->window, row1, col_width*1 + col_width/2 - strlen(pause)/2, pause);
  mvwprintw(main_window->window, row1, col_width*2 + col_width/2 - strlen(resume)/2, resume);
  mvwprintw(main_window->window, row1, col_width*3 + col_width/2 - strlen(stop)/2, stop);

  mvwprintw(main_window->window, row2, col_width*0 + col_width/2 - strlen(change_spage)/2, change_spage);
  mvwprintw(main_window->window, row2, col_width*1 + col_width/2 - strlen(change_dpage)/2, change_dpage);
  mvwprintw(main_window->window, row2, col_width*2 + col_width/2 - strlen(back)/2, back);
  mvwprintw(main_window->window, row2, col_width*3 + col_width/2 - strlen(quit)/2, quit);

  // render the trace screen
  wrefresh(main_window->window);
  wrefresh(spage_window->window);
  wrefresh(dpage_window->window);
  wrefresh(hist_window->window);
  prefresh(trace_windows.spage_text_field, spage_min_row, spage_min_col, spage_view_top, spage_view_left, spage_view_bot, spage_view_right);
  prefresh(trace_windows.dpage_text_field, dpage_min_row, dpage_min_col, dpage_view_top, dpage_view_left, dpage_view_bot, dpage_view_right);
  prefresh(history.text_field, history.min_row, history.min_col, history.view_top, history.view_left, history.view_bot, history.view_right);

  // handle actions via key presses
  keypad(main_window->window, TRUE);
  wtimeout(main_window->window, 100);   // errors out wgetch after 100 ms, falls to defualt
  while (1) {
    int ch = wgetch(main_window->window);
    int index;

    switch(ch) {
      case 's':
      case 'S':
        focus_window(trace_windows.hist_window, true);
        init_trace_verification();
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
        pthread_mutex_lock(&trace_data.lock);
        index = strlen(trace_data.start_page);
        pthread_mutex_unlock(&trace_data.lock);
        focus_window(trace_windows.hist_window, false);
        read_user_input(1, trace_windows.spage_text_field, spage_min_row, spage_min_col, spage_view_top, spage_view_left, spage_view_bot, spage_view_right, index);
        break;
      case 't':
      case 'T':
        pthread_mutex_lock(&trace_data.lock);
        index = strlen(trace_data.dest_page);
        pthread_mutex_unlock(&trace_data.lock);
        focus_window(trace_windows.hist_window, false);
        read_user_input(2, trace_windows.dpage_text_field, dpage_min_row, dpage_min_col, dpage_view_top, dpage_view_left, dpage_view_bot, dpage_view_right, index);
        break;
      case 'b':
      case 'B':
        delwin(main_window->window);
        delwin(spage_window->window);
        delwin(trace_windows.spage_text_field);
        delwin(dpage_window->window);
        delwin(trace_windows.dpage_text_field);
        delwin(hist_window->window);
        delwin(history.text_field);
        init_view();
        break;
      case 'q':
      case 'Q':
        delwin(main_window->window);
        delwin(spage_window->window);
        delwin(trace_windows.spage_text_field);
        delwin(dpage_window->window);
        delwin(trace_windows.dpage_text_field);
        delwin(hist_window->window);
        delwin(history.text_field);
        endwin();
        return;
      default:
        break;
    }

    update_trace_verification();    // checks every 100ms to see if worker finished
  }
}


/*
 * Sends of a worker to verify the pages provided exist
 */
void init_trace_verification() {
  pthread_join(worker, NULL);

  // show immediate feedback
  pthread_mutex_lock(&trace_data.lock);
  TraceHistory history = trace_data.history;
  wclear(history.text_field);
  mvwprintw(history.text_field, 1, 2, "Verifying pages...");
  prefresh(history.text_field, history.min_row, history.min_col, history.view_top, history.view_left, history.view_bot, history.view_right);
  pthread_mutex_unlock(&trace_data.lock);

  pthread_create(&worker, NULL, verify_pages, &trace_data);
}


/*
 * Continuously checks struct shared with worker to see if its completed
 */
void update_trace_verification() {
  pthread_mutex_lock(&trace_data.lock);
  if (trace_data.status == 1) {    // missing input
    trace_data.status = -1;
    TraceHistory history = trace_data.history;
    wclear(history.text_field);
    char* invalid_msg = "Missing page titles. Please enter two Wikipedia pages.";
    mvwprintw(history.text_field, 1, 2, invalid_msg);
    prefresh(history.text_field, history.min_row, history.min_col, history.view_top, history.view_left, history.view_bot, history.view_right);
  }
  else if (trace_data.status == 2) {   // cJSON parse error
    trace_data.status = -1;
    TraceHistory history = trace_data.history;
    wclear(history.text_field);
    mvwprintw(history.text_field, 1, 2, trace_data.err_message);
    prefresh(history.text_field, history.min_row, history.min_col, history.view_top, history.view_left, history.view_bot, history.view_right);
  }
  else if (trace_data.status == 3) {   // page doesnt exists
    trace_data.status = -1;
    TraceHistory history = trace_data.history;
    wclear(history.text_field);
    mvwprintw(history.text_field, 1, 2, trace_data.err_message);
    prefresh(history.text_field, history.min_row, history.min_col, history.view_top, history.view_left, history.view_bot, history.view_right);
  }
  else if (trace_data.status == 0) {    // valid pages 
    trace_data.status = -1;
    TraceHistory history = trace_data.history;
    wclear(history.text_field);

    // TODO

    /* endwin(); */
    /* printf("verified start page: %s\n", trace_data.start_page); */
    /* printf("verified dest page: %s\n", trace_data.dest_page); */ 
    /* exit(0); */

    prefresh(history.text_field, history.min_row, history.min_col, history.view_top, history.view_left, history.view_bot, history.view_right);
  }
  pthread_mutex_unlock(&trace_data.lock);
}


/*
 * Read chars entered into the start page text field or the destination page text field.
 * 
 * @param page: a flag to know which text field is being written to (1 = spage, 2 = dpage)
 * @param text_field: the window to display and read text from
 * @param min_row: the starting row displayed in the window
 * @param min_col: the staring column displyed in the window
 * @param view_top: location of the top of the window
 * @param view_left: location of the left of the window
 * @param view_bot: location of the bottom of the window
 * @param view_right: location of the right of the window
 * @param index: the current index of the cursor and buffer
 */
static void read_user_input(int page, WINDOW* text_field, int min_row, int min_col, int view_top, int view_left, int view_bot, int view_right, int index) {
  curs_set(1);
  wmove(text_field, 0, index);

  // change border color to show avtive window
  if (page == 1) { focus_window(trace_windows.spage_window, true); }
  else { focus_window(trace_windows.dpage_window, true);

  }

  prefresh(text_field, 0, 0, view_top, view_left, view_bot, view_right);
  keypad(text_field, TRUE);

  // prefill memory if editing populated text field
  char text[256] = {0};
  pthread_mutex_lock(&trace_data.lock);
  if (page == 1) { strcpy(text, trace_data.start_page); }
  else { strcpy(text, trace_data.dest_page); }
  pthread_mutex_unlock(&trace_data.lock);

  while(1) {
    int ch = wgetch(text_field);

    // ASCCII chars
    if (ch >= 32 && ch <= 126 && index <= 255) {
      waddch(text_field, ch);   // add to text field
      text[index] = ch;    // store it
      index++;    // increment cursor and next spot to fill in buffer
      update_text_field_view(text_field, min_row, min_col, view_top, view_left, view_bot, view_right, index);
    }
    // deleting chars
    else if ( (ch == KEY_BACKSPACE || ch == 127 || ch == 8) && index >= 0) {
      if (index == 0) {   // deletes the last char to completely clear text field
        waddch(text_field, ' ');
        wmove(text_field, 0, index);
        prefresh(text_field, 0, 0, view_top, view_left, view_bot, view_right);
        continue;
      }
      index--;
      mvwaddch(text_field, 0, index, ' ');
      text[index] = '\0';
      update_text_field_view(text_field, min_row, min_col, view_top, view_left, view_bot, view_right, index);
    }
    // user pressed enter
    else if (ch == 10) {
      // change window border color
      if (page == 1) {
        focus_window(trace_windows.spage_window, false);
        update_text_field_view(text_field, min_row, min_col, view_top, view_left, view_bot, view_right, index);
        curs_set(0);
      }
      else {
        focus_window(trace_windows.dpage_window, false);
        update_text_field_view(text_field, min_row, min_col, view_top, view_left, view_bot, view_right, index);
        curs_set(0);
      }

      text[index] = '\0';
      pthread_mutex_lock(&trace_data.lock);
      if (page == 1) { strcpy(trace_data.start_page, text); }
      else { strcpy(trace_data.dest_page, text); }
      pthread_mutex_unlock(&trace_data.lock);

      return;
    }
  }
}


/*
 * Updates the text field view when user types in the start page or destination page text field
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
 * bring a window into focus by changing its border color
 */
void focus_window(WindowProps window_props, bool focus) {
  if (focus) {
    wattron(window_props.window, COLOR_PAIR(1));
    box(window_props.window, 0, 0);
    mvwprintw(window_props.window, 0, 2, window_props.title);
    wattroff(window_props.window, COLOR_PAIR(1));
    wrefresh(window_props.window);
  }
  else {
    box(window_props.window, 0, 0);
    mvwprintw(window_props.window, 0, 2, window_props.title);
    wrefresh(window_props.window);
  }
}


/*
 *
 */


/*
 * Used to update the trace history view.
 *
 * @param history: a struct containing all necesary fields to update its display text
 */
void update_trace_history(TraceHistory history) {
  // TODO
}


/*
 * Display screen that allows user to adjust some settings of the tracer.
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
  WindowProps* about_window = &about_windows.main_window;
  int about_start_y = (LINES - ABOUT_WIN_HEIGHT) / 2;
  int about_start_x = (COLS - ABOUT_WIN_WIDTH) / 2;
  about_window->window = newwin(ABOUT_WIN_HEIGHT, ABOUT_WIN_WIDTH, about_start_y, about_start_x);
  keypad(about_window->window, TRUE);
  box(about_window->window, 0, 0);

  // title of window
  about_window->title = "ABOUT";
  wattron(about_window->window, A_BOLD);
  mvwprintw(about_window->window, 0, ABOUT_WIN_WIDTH/2 - strlen(about_window->title)/2, about_window->title);
  wattroff(about_window->window, A_BOLD);

  // description of window
  mvwprintw(about_window->window, 1, 3, "Share details about the Wikipedia trace game and how this program works to find");
  mvwprintw(about_window->window, 2, 3, "potential paths between a starting page to a destination page");

  // actions at the bottom
  char* back = "(b) back";
  char* quit = "(q) quit";
  mvwprintw(about_window->window, ABOUT_WIN_HEIGHT - 3, ABOUT_WIN_WIDTH/4 - strlen(back)/2, back);
  mvwprintw(about_window->window, ABOUT_WIN_HEIGHT - 3, ABOUT_WIN_WIDTH/2 + ABOUT_WIN_WIDTH/4 - strlen(quit)/2, quit);

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
  wrefresh(about_window->window);
  prefresh(about_windows.text_field, 
           pad_scroll_line, 0,
           view_top, view_left,
           view_bot, view_right);

  // scrollable view of information
  while (1) {
    wrefresh(about_window->window);
    prefresh(about_windows.text_field,
             pad_scroll_line, 0,
             view_top, view_left,
             view_bot, view_right);

    int ch = wgetch(about_window->window);
    switch(ch) {
      case KEY_UP:
        if (pad_scroll_line > 0) { pad_scroll_line--; }
        break;
      case KEY_DOWN:
        if (pad_scroll_line < (ABOUT_PAD_ROWS - view_height)) { pad_scroll_line++; }
        break;
      case 'b':
      case 'B':
        delwin(about_window->window);
        delwin(about_windows.text_field);
        init_view();
        return;
      case 'q':
      case 'Q':
        delwin(about_window->window);
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

