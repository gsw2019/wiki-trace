/**
 * function prototypes for the ncurses user interface of wiki-trace
 *
 */


#include <ncurses.h>

#ifndef VIEW_H
#define VIEW_H


#define WELCOME_WIN_HEIGHT     5
#define WELCOME_WIN_WIDTH      75
#define AUTHOR_WIN_HEIGHT      1
#define AUTHOR_WIN_WIDTH       75
#define MENU_WIN_HEIGHT        10
#define MENU_WIN_WIDTH         75
#define OPT_START_TRACE        0
#define OPT_SETTINGS           1
#define OPT_ABOUT              2
#define OPT_EXIT               3

#define TRACE_WIN_HEIGHT       40
#define TRACE_WIN_WIDTH        85
#define TEXT_WIN_HEIGHT        3
#define TEXT_WIN_WIDTH         35
#define TEXT_FIELD_HEIGHT      1
#define TEXT_FIELD_WIDTH       255
#define HIST_WIN_HEIGHT        25
#define HIST_WIN_WIDTH         70
#define HIST_TEXT_FIELD_HEIGHT 500
#define HIST_TEXT_FIELD_WIDTH  65

#define ABOUT_WIN_HEIGHT       40
#define ABOUT_WIN_WIDTH        85
#define ABOUT_PAD_ROWS         100
#define ABOUT_PAD_COLS         75


typedef struct {
  char start_page[256];
  char dest_page[256];
} TracePages;

typedef struct {
  WINDOW* text_field;
  int min_row;
  int min_col;
  int view_top;
  int view_left;
  int view_bot;
  int view_right;
} TraceHistory;

typedef struct {
  WINDOW* welcome_window;
  WINDOW* author_window;
  WINDOW* menu_window;
} InitWindows;

typedef struct {
  WINDOW* main_window;
  WINDOW* spage_window;
  WINDOW* spage_text_field;
  WINDOW* dpage_window;
  WINDOW* dpage_text_field;
  WINDOW* hist_window; 
} TraceWindows;

typedef struct {

} SettingsWindows;

typedef struct {
  WINDOW* main_window;
  WINDOW* text_field;
} AboutWindows;


// functions concerened with rendering start screen
void init_wiki_trace_view();
static void show_welcome();
static void show_author();
static void show_menu();
static void update_menu(int highlighted);

// functions concerned with rendering trace screen
static void show_trace();
static void show_trace_history(WINDOW* trace_window, int trace_window_start_y);
static void read_user_input(WINDOW* text_field, int min_row, int min_col, int view_top, int view_left, int view_bot, int view_right, int index);
static void update_text_field_view(WINDOW* text_field, int min_row, int min_col, int view_top, int view_left, int view_bot, int view_right, int index);
void update_trace_history();
TracePages get_trace_pages();
TraceHistory get_trace_history();

// functions concerned with rendering settings screen
static void show_settings();

// functions concerned with rendering about screen
static void show_about();


#endif

