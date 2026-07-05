/**
 * Implementations of view.h prototypes. Renders a TUI that has 4 different views: launch screen,
 * trace screen, settings screen, and an about screen.
 *
 * In order for the TUI to remain rendered, subsequent work such as fetching data and running
 * the tracing algorithm are ran through seperate threads.
 *
 * @author Garret Wilson
 */


#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

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
TraceData trace_data = {   // shared data struct for view, fetcher, and tracer
  .start_page = {0},
  .dest_page = {0},
  .hops = 0,
  .init_complete = 0,
  .trace_complete = 0,
  .trace_successful = 0,
  .status = 0,
  .lock = PTHREAD_MUTEX_INITIALIZER
};

time_t start_message_delay = 0;

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
  mvwprintw(init_windows.welcome_window, 2, WELCOME_WIN_WIDTH/2 - strlen(welcome_message)/2, "%s", welcome_message);
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
  mvwprintw(init_windows.author_window, 0, AUTHOR_WIN_WIDTH/2 - strlen(author_tag)/2, "%s", author_tag);
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
      return;
    case OPT_SETTINGS:
      // TODO
      return;
    case OPT_ABOUT:
      delwin(init_windows.menu_window);
      delwin(init_windows.author_window);
      delwin(init_windows.welcome_window);
      show_about();
      return;
    case OPT_EXIT:
      clear();
      refresh();
      endwin();
      return;
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
    // highlight the option the cursor is on
    if (highlighted == i + 1) {   // highlighted is 1 indexed
      wattron(init_windows.menu_window, A_REVERSE);
      mvwprintw(init_windows.menu_window, y, x, "%s", choices[i]);
      wattroff(init_windows.menu_window, A_REVERSE);
    } 
    else {
      mvwprintw(init_windows.menu_window, y, x, "%s", choices[i]);
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
  mvwprintw(main_window->window, 0, TRACE_WIN_WIDTH/2 - strlen(main_window->title)/2, "%s", main_window->title); 
  wattroff(main_window->window, A_BOLD);

  char* header_message1 = "Walk one of the possible paths that exists between the start Wikipedia page and";
  char* header_message2 = "the destination Wikipedia page. Pages that are extremely different will take";
  char* header_message3 = "longer to search for a path, if one exists.";
  mvwprintw(main_window->window, 1, TRACE_WIN_WIDTH/2 - strlen(header_message1)/2, header_message1);
  mvwprintw(main_window->window, 2, TRACE_WIN_WIDTH/2 - strlen(header_message2)/2, header_message2);
  mvwprintw(main_window->window, 3, TRACE_WIN_WIDTH/2 - strlen(header_message3)/2, header_message3);

  // arrow pointing from start page to destination page
  wattron(main_window->window, A_BOLD);
  mvwprintw(main_window->window, 6, TRACE_WIN_WIDTH/2 - strlen("======>")/2, "%s", "======>");
  wattroff(main_window->window, A_BOLD);

  // inner window for start page (spage)
  WindowProps* spage_window = &trace_windows.spage_window;
  int spage_window_y = trace_window_start_y + 5;
  int spage_window_x = trace_window_start_x + 2;
  spage_window->window = newwin(TEXT_WIN_HEIGHT, TEXT_WIN_WIDTH, spage_window_y, spage_window_x);

  spage_window->title = "start_page";
  box(spage_window->window, 0, 0);
  mvwprintw(spage_window->window, 0, 2, "%s", spage_window->title);

  // text field for spage window
  WindowProps* spage_text_field = &trace_windows.spage_text_field;
  spage_text_field->window = newpad(TEXT_FIELD_HEIGHT, TEXT_FIELD_WIDTH);
  spage_text_field->min_row = 0;
  spage_text_field->min_col = 0;
  spage_text_field->view_top = spage_window_y + 1;
  spage_text_field->view_left = spage_window_x + 1;
  spage_text_field->view_bot = spage_window_y + 1;
  spage_text_field->view_right = spage_window_x + TEXT_WIN_WIDTH - 2;

  // inner window for destination page (dpage)
  WindowProps* dpage_window = &trace_windows.dpage_window;
  int dpage_window_y = trace_window_start_y + 5;
  int dpage_window_x = trace_window_start_x + TRACE_WIN_WIDTH - TEXT_WIN_WIDTH - 2;
  dpage_window->window = newwin(TEXT_WIN_HEIGHT, TEXT_WIN_WIDTH, dpage_window_y, dpage_window_x);

  dpage_window->title = "destination page";
  box(dpage_window->window, 0, 0);
  mvwprintw(dpage_window->window, 0, 2, "%s", dpage_window->title);

  // text field for dpage window
  WindowProps* dpage_text_field = &trace_windows.dpage_text_field;
  dpage_text_field->window = newpad(TEXT_FIELD_HEIGHT, TEXT_FIELD_WIDTH);
  dpage_text_field->min_row = 0;
  dpage_text_field->min_col = 0;
  dpage_text_field->view_top  = dpage_window_y + 1;
  dpage_text_field->view_left = dpage_window_x + 1;
  dpage_text_field->view_bot  = dpage_window_y + 1;
  dpage_text_field->view_right = dpage_window_x + TEXT_WIN_WIDTH - 2;

  // inner window for trace history
  WindowProps* hist_window = &trace_windows.hist_window;
  int hist_start_y = trace_window_start_y + 9;
  int hist_start_x = trace_window_start_x + (TRACE_WIN_WIDTH - HIST_WIN_WIDTH) / 2;
  hist_window->window = newwin(HIST_WIN_HEIGHT, HIST_WIN_WIDTH, hist_start_y, hist_start_x);

  hist_window->title = "trace history";
  box(hist_window->window, 0, 0);
  mvwprintw(hist_window->window, 0, 2, "%s", hist_window->title);

  // text field for trace history
  WindowProps* hist_text_field = &trace_windows.hist_text_field;
  hist_text_field->window = newpad(HIST_TEXT_FIELD_HEIGHT, HIST_TEXT_FIELD_WIDTH);
  hist_text_field->min_row = 0;
  hist_text_field->min_col = 0;
  hist_text_field->view_top = hist_start_y + 1;
  hist_text_field->view_left = hist_start_x + 2;
  hist_text_field->view_bot = hist_start_y + HIST_WIN_HEIGHT - 2;
  hist_text_field->view_right = hist_start_x + HIST_WIN_WIDTH - 2;
  hist_text_field->write_row = 1;
  hist_text_field->write_col = 2;

  for (int i = 0; i < HIST_TEXT_FIELD_HEIGHT; i++) {
    mvwprintw(hist_text_field->window, i, 1, "this is entry #%d", i);
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

  mvwprintw(main_window->window, row1, col_width*0 + col_width/2 - strlen(start)/2, "%s", start);
  mvwprintw(main_window->window, row1, col_width*1 + col_width/2 - strlen(pause)/2, "%s", pause);
  mvwprintw(main_window->window, row1, col_width*2 + col_width/2 - strlen(resume)/2, "%s", resume);
  mvwprintw(main_window->window, row1, col_width*3 + col_width/2 - strlen(stop)/2, "%s", stop);

  mvwprintw(main_window->window, row2, col_width*0 + col_width/2 - strlen(change_spage)/2, "%s", change_spage);
  mvwprintw(main_window->window, row2, col_width*1 + col_width/2 - strlen(change_dpage)/2, "%s", change_dpage);
  mvwprintw(main_window->window, row2, col_width*2 + col_width/2 - strlen(back)/2, "%s", back);
  mvwprintw(main_window->window, row2, col_width*3 + col_width/2 - strlen(quit)/2, "%s", quit);

  // fill fields if previously entered
  fill_text_fields();

  // render the trace screen
  wrefresh(main_window->window);
  wrefresh(spage_window->window);
  wrefresh(dpage_window->window);
  wrefresh(hist_window->window);
  prefresh(spage_text_field->window,
           spage_text_field->min_row, spage_text_field->min_col,
           spage_text_field->view_top, spage_text_field->view_left,
           spage_text_field->view_bot, spage_text_field->view_right);
  prefresh(dpage_text_field->window,
           dpage_text_field->min_row, dpage_text_field->min_col,
           dpage_text_field->view_top, dpage_text_field->view_left,
           dpage_text_field->view_bot, dpage_text_field->view_right);
  prefresh(hist_text_field->window,
           hist_text_field->min_row, hist_text_field->min_col,
           hist_text_field->view_top, hist_text_field->view_left,
           hist_text_field->view_bot, hist_text_field->view_right);

  keypad(main_window->window, TRUE);    // handle actions via key presses
  wtimeout(main_window->window, 100);   // errors out wgetch after 100 ms, falls to defualt
  int prev_hops = 0;

  while (1) {
    int ch = wgetch(main_window->window);

    switch(ch) {
      case 's':
      case 'S':
        focus_window(&trace_windows.hist_window, true);
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
        focus_window(&trace_windows.hist_window, false);
        read_user_input(1, spage_text_field);
        break;
      case 't':
      case 'T':
        focus_window(&trace_windows.hist_window, false);
        read_user_input(2, dpage_text_field);
        break;
      case 'b':
      case 'B':
        delwin(main_window->window);
        delwin(spage_window->window);
        delwin(spage_text_field->window);
        delwin(dpage_window->window);
        delwin(dpage_text_field->window);
        delwin(hist_window->window);
        delwin(hist_window->window);
        init_view();
        return;
      case 'q':
      case 'Q':
        delwin(main_window->window);
        delwin(spage_window->window);
        delwin(spage_text_field->window);
        delwin(dpage_window->window);
        delwin(dpage_text_field->window);
        delwin(hist_window->window);
        delwin(hist_window->window);
        endwin();
        return;
      default:
        break;
    }

    // checks every 100ms to see if verification worker finished
    pthread_mutex_lock(&trace_data.lock);
    if (trace_data.init_complete == 1) {
      trace_data.init_complete = 0;
      if (peek_worker_status() == 0) {
        pthread_mutex_unlock(&trace_data.lock);
        show_start_message();
        continue;
      }
    }
    pthread_mutex_unlock(&trace_data.lock);

    // 2 seconds after shwoing start message begin the trace
    if ((start_message_delay != 0) && (time(NULL) - start_message_delay >= 2)) {
      start_message_delay = 0;
      start_trace();
    }

    // checks every 100ms to see if can update trace history
    pthread_mutex_lock(&trace_data.lock);
    if (trace_data.hops > prev_hops) {
      // go through prev_hops + 1 to hops
      // print them
      // update prev_hops
      update_trace_history(&prev_hops);
    }
    pthread_mutex_unlock(&trace_data.lock);

    // checks every 100ms to see if trace worker finished
    pthread_mutex_lock(&trace_data.lock);
    if (trace_data.trace_complete == 1) {
      trace_data.trace_complete = 0;
      peek_worker_status();
    }
    pthread_mutex_unlock(&trace_data.lock);
  }
}


/*
 * Fill text fields if there is memory for start and destination page.
 */
static void fill_text_fields() {
  pthread_mutex_lock(&trace_data.lock);
  if (strlen(trace_data.start_page)) {
    wprintw(trace_windows.spage_text_field.window, "%s", trace_data.start_page);
  }
  if (strlen(trace_data.dest_page)) {
    wprintw(trace_windows.dpage_text_field.window, "%s", trace_data.dest_page);
  }
  pthread_mutex_unlock(&trace_data.lock);
}


/*
 * Sends of a worker to verify the pages provided exist.
 */
static void init_trace_verification() {
  pthread_join(worker, NULL);

  // show immediate feedback
  WindowProps* history = &trace_windows.hist_text_field;
  wclear(history->window);
  mvwprintw(history->window, 1, history->write_col, "%s", "Verifying pages...");
  prefresh(history->window,
           history->min_row, history->min_col,
           history->view_top, history->view_left,
           history->view_bot, history->view_right);

  pthread_create(&worker, NULL, verify_pages, NULL);
}


/*
 * Continuously checks struct shared with worker to see if its completed. Wrapped in
 * mutex where it is called.
 */
static int peek_worker_status() {
  if (trace_data.status == ERROR_INPUT) {    // missing input
    trace_data.status = 0;
    WindowProps* history = &trace_windows.hist_text_field;
    wclear(history->window);
    mvwprintw(history->window, 1, history->write_col, "%s", trace_data.err_message);
    prefresh(history->window,
             history->min_row, history->min_col,
             history->view_top, history->view_left,
             history->view_bot, history->view_right);
    return 1;
  }
  else if (trace_data.status == ERROR_MEM) {    // error with malloc
    trace_data.status = 0;
    WindowProps* history = &trace_windows.hist_text_field;
    wclear(history->window);
    mvwprintw(history->window, 1, history->write_col, "%s", trace_data.err_message);
    prefresh(history->window,
             history->min_row, history->min_col,
             history->view_top, history->view_left,
             history->view_bot, history->view_right);
    return 1;
  }
  else if (trace_data.status == ERROR_PARSE) {   // cJSON parse error
    trace_data.status = 0;
    WindowProps* history = &trace_windows.hist_text_field;
    wclear(history->window);
    mvwprintw(history->window, 1, history->write_col, "%s", trace_data.err_message);
    prefresh(history->window,
             history->min_row, history->min_col,
             history->view_top, history->view_left,
             history->view_bot, history->view_right);
    return 1;
  }
  else if (trace_data.status == ERROR_EXISTENCE) {   // page doesnt exists
    trace_data.status = 0;
    WindowProps* history = &trace_windows.hist_text_field;
    wclear(history->window);
    mvwprintw(history->window, 1, history->write_col, "%s", trace_data.err_message);
    prefresh(history->window,
             history->min_row, history->min_col,
             history->view_top, history->view_left,
             history->view_bot, history->view_right);
    return 1;
  }

  return 0;
}


/*
 * displays a starting message and starts timer to launch trace
 */
static void show_start_message() {
  // write message trace is starting
  WindowProps* history = &trace_windows.hist_text_field;
  wclear(history->window);
  mvwprintw(history->window, 1, history->write_col, "%s", "Starting trace...");
  prefresh(history->window,
           history->min_row, history->min_col,
           history->view_top, history->view_left,
           history->view_bot, history->view_right);

  // initialze timer
  start_message_delay = time(NULL);
}


/*
 * Sends off a worker to begin the trace and prints starting page
 */
static void start_trace() {
  pthread_join(worker, NULL);

  pthread_mutex_lock(&trace_data.lock);
  char* start_page = strdup(trace_data.start_page);
  pthread_mutex_unlock(&trace_data.lock);

  WindowProps* history = &trace_windows.hist_text_field;
  wclear(history->window);
  mvwprintw(history->window, history->write_row, history->write_col, "%s", start_page);
  prefresh(history->window,
           history->min_row, history->min_col,
           history->view_top, history->view_left,
           history->view_bot, history->view_right);
  history->write_row++;

  pthread_create(&worker, NULL, run_trace, NULL);
}


/*
 * Read chars entered into the start page text field or the destination page text field.
 *
 * @param page: a flag to know which text field is being written to (1 = spage, 2 = dpage)
 * @param text_field: the window data needed to display and read text from
 */
static void read_user_input(int page, WindowProps* text_field) {
  keypad(text_field->window, TRUE);

  // change border color to show avtive window
  if (page == 1) { focus_window(&trace_windows.spage_window, true); }
  else { focus_window(&trace_windows.dpage_window, true); }

  // prefill memory if editing populated text field
  char text[256] = {0};
  int index;
  pthread_mutex_lock(&trace_data.lock);
  if (page == 1) {
    strcpy(text, trace_data.start_page);
    index = strlen(trace_data.start_page);
  }
  else {
    strcpy(text, trace_data.dest_page);
    index = strlen(trace_data.dest_page);
  }
  pthread_mutex_unlock(&trace_data.lock);

  curs_set(1);
  wmove(text_field->window, 0, index);
  prefresh(text_field->window, 0, 0, text_field->view_top, text_field->view_left, text_field->view_bot, text_field->view_right);

  while(1) {
    int ch = wgetch(text_field->window);

    // ASCCII chars
    if (ch >= 32 && ch <= 126 && index <= 255) {
      waddch(text_field->window, ch);   // add to text field
      text[index] = ch;    // store it
      index++;    // increment cursor and next spot to fill in buffer
      update_text_field(text_field, index);
    }
    // deleting chars
    else if ( (ch == KEY_BACKSPACE || ch == 127 || ch == 8) && index >= 0) {
      if (index == 0) {   // deletes the last char to completely clear text field
        waddch(text_field->window, ' ');
        wmove(text_field->window, 0, index);
        prefresh(text_field->window, 0, 0, text_field->view_top, text_field->view_left, text_field->view_bot, text_field->view_right);
        continue;
      }
      index--;
      mvwaddch(text_field->window, 0, index, ' ');
      text[index] = '\0';
      update_text_field(text_field, index);
    }
    // user pressed enter
    else if (ch == 10) {
      // change window border color
      if (page == 1) {
        focus_window(&trace_windows.spage_window, false);
        update_text_field(text_field, index);
        curs_set(0);
      }
      else {
        focus_window(&trace_windows.dpage_window, false);
        update_text_field(text_field, index);
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
 * @param text_field: the window data needed to display and read text from
 * @param index: the current index of the cursor and buffer
 */
static void update_text_field(WindowProps* text_field, int index) {
  int new_min_col;

  if (index >= TEXT_WIN_WIDTH - 2) {
    new_min_col = index - TEXT_WIN_WIDTH + 2;
  }
  else {
    new_min_col = 0;
  }

  wmove(text_field->window, 0, index);
  prefresh(text_field->window, text_field->min_row, new_min_col, text_field->view_top, text_field->view_left, text_field->view_bot, text_field->view_right);
}


/*
 * bring a window into focus by changing its border color
 */
static void focus_window(WindowProps* window_props, bool focus) {
  // color to draw active window borders
  init_pair(1, COLOR_CYAN, -1);

  if (focus) {
    wattron(window_props->window, COLOR_PAIR(1));
    box(window_props->window, 0, 0);
    mvwprintw(window_props->window, 0, 2, "%s", window_props->title);
    wattroff(window_props->window, COLOR_PAIR(1));
    wrefresh(window_props->window);
  }
  else {
    box(window_props->window, 0, 0);
    mvwprintw(window_props->window, 0, 2, "%s", window_props->title);
    wrefresh(window_props->window);
  }
}


/*
 * Used to update the trace history view.
 */
void update_trace_history(int* prev_hops) {
  WindowProps* history = &trace_windows.hist_text_field;

  for (int i = *prev_hops; i < trace_data.hops; i++) {
    mvwprintw(history->window, history->write_row, history->write_col, "%s", trace_data.pages_traveled[i]);
    prefresh(history->window,
           history->min_row, history->min_col,
           history->view_top, history->view_left,
           history->view_bot, history->view_right);
    history->write_row++;
  }

  if (trace_data.trace_complete == 1) {
    focus_window(history, false);
  }

  *prev_hops = trace_data.hops;
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
  mvwprintw(about_window->window, 0, ABOUT_WIN_WIDTH/2 - strlen(about_window->title)/2, "%s", about_window->title);
  wattroff(about_window->window, A_BOLD);

  // description of window
  mvwprintw(about_window->window, 1, 3, "%s", "Share details about the Wikipedia trace game and how this program works to find");
  mvwprintw(about_window->window, 2, 3, "%s", "potential paths between a starting page to a destination page");

  // actions at the bottom
  char* back = "(b) back";
  char* quit = "(q) quit";
  mvwprintw(about_window->window, ABOUT_WIN_HEIGHT - 3, ABOUT_WIN_WIDTH/4 - strlen(back)/2, "%s", back);
  mvwprintw(about_window->window, ABOUT_WIN_HEIGHT - 3, ABOUT_WIN_WIDTH/2 + ABOUT_WIN_WIDTH/4 - strlen(quit)/2, "%s", quit);

  // creating a scrollable interior
  WindowProps* about_text_field = &about_windows.text_field;
  about_text_field->window = newpad(ABOUT_PAD_ROWS, ABOUT_PAD_COLS);

  // write aactual info to the scrollable area
  // TODO
  for (int i = 0; i < ABOUT_PAD_ROWS; i++) {
    mvwprintw(about_text_field->window, i, 1, "here is a temp entery inside this scrollaable area that I want #%d", i + 1);
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
  prefresh(about_text_field->window, 
           pad_scroll_line, 0,
           view_top, view_left,
           view_bot, view_right);

  // scrollable view of information
  while (1) {
    wrefresh(about_window->window);
    prefresh(about_text_field->window,
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
        delwin(about_text_field->window);
        init_view();
        return;
      case 'q':
      case 'Q':
        delwin(about_window->window);
        delwin(about_text_field->window);
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

