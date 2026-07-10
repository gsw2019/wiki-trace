/**
 * Prototypes, macros, and struts for the ncurses user interface of wiki-trace
 *
 * @author Garret Wilson
 */


#ifndef VIEW_H
#define VIEW_H


#include <ncurses.h>
#include <pthread.h>


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


// tracks all properties that a window might have
typedef struct {
  WINDOW* window;
  char* title;
  int min_row;
  int min_col;
  int view_top;
  int view_left;
  int view_bot;
  int view_right;
  int write_row;
  int write_col;
} WindowProps;

// carries info over threads
typedef struct {
  char start_page[256];
  char dest_page[256];
  char** pages_traveled;
  int num_pages_traveled;
  int init_complete;
  int trace_complete;
  int trace_successful;
  int status;
  char* err_message;
  int quit_request;
  int pause_request;
  int resume_request;
  pthread_mutex_t lock;
} TraceData;

// tracks all launch screen windows
typedef struct {
  WINDOW* welcome_window;
  WINDOW* author_window;
  WINDOW* menu_window;
} InitWindows;

// tracks all trace screen windows
typedef struct {
  WindowProps main_window;
  WindowProps spage_window;
  WindowProps spage_text_field;
  WindowProps dpage_window;
  WindowProps dpage_text_field;
  WindowProps hist_window;
  WindowProps hist_text_field;
} TraceWindows;

// tracks all settings screen windows
typedef struct {
  // TODO
} SettingsWindows;

// tracks all about screen windows
typedef struct {
  WindowProps main_window;
  WindowProps text_field;
} AboutWindows;


// functions concerened with rendering start screen
void init_view();
static void show_welcome();
static void show_author();
static void show_menu();
static void update_menu(int highlighted);

// functions concerned with rendering trace screen
static void show_trace();
static void fill_text_fields();
static void read_user_input(int page, WindowProps* text_field);
static void update_text_field(WindowProps* text_field, int index);
static void focus_window(WindowProps* window_props, bool focus);
void update_trace_history();

// functions concerened with starting, pausing, or stopping the trace
static void init_trace_verification();
static int peek_worker_status(int status);
static void show_start_message();
static void start_trace();

// functions concerned with rendering settings screen
static void show_settings();

// functions concerned with rendering about screen
static void show_about();


#endif

