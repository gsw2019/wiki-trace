/**
 * Prototypes, structs, and macros for for fetching Wikipedia data
 *
 * @author Garret Wilson
 */


#include <curl/curl.h>

#include "view.h"
#include "cJSON.h"


#define URL_LEN              500

#define ERROR_MEM            1
#define ERROR_INPUT          2
#define ERROR_PARSE          3
#define ERROR_EXISTENCE      4

#define INIT_LINK_ARRAY_SIZE 2


typedef struct {
  char* verify_start;
  char* verify_end;
  char* links_start;
  char* links_end;
  char* links_cont_start;
  char* links_cont_mid;
  char* intro_start;
  char* intro_end;
} URLParts;

typedef struct {
  char *data;
  size_t size;
} Response;

typedef struct {
  char* title;
  char* intro;
  char* content;
  char** links;
} PageData;


extern TraceData trace_data;    // global var used to track trace state across view.c and fetcher.c


// init or utility funcs
CURL* init_curl();
static size_t write_callback(void *ptr, size_t size, size_t nmemb, Response *res);

// verifying pages input by user
void* verify_pages(void* args);
static int check_page_exists(char* page_data);

// starting up the trace
void* run_trace(void* args);
static void get_page_links(char* page_title);
static void get_page_content(TraceData* trace_data);
static void parse_links(cJSON* json_data, PageData* page_data);

