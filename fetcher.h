/**
 * Prototypes, structs, and macros for for fetching Wikipedia data
 *
 * @author Garret Wilson
 */


#ifndef FETCHER_H
#define FETCHER_H


#include <curl/curl.h>

#include "view.h"
#include "cJSON.h"


#define URL_LEN              500

#define INIT_DATA_ARRAY_SIZE 2


typedef struct {
  const char* verify_start;
  const char* verify_end;
  const char* links_start;
  const char* links_end;
  const char* links_cont_start;
  const char* links_cont_mid;
  const char* intro_start;
  const char* intro_end;
  const char* intro_cont_start;
  const char* intro_cont_mid;
  const char* content_start;
  const char* content_end;
} URLParts;

typedef struct {
  char *data;
  size_t size;
} Response;

typedef struct {
  char* title;
  char* content;
  char** links_titles;
  cJSON** links_data;
  int num_links;
  int num_links_data;
  int capacity_links;
} PageData;


extern TraceData trace_data;    // global var used to track trace state across view.c and fetcher.c


// init or utility funcs
CURL* init_curl();
static size_t write_callback(void *ptr, size_t size, size_t nmemb, Response *res);

// verifying pages input by user
void* verify_pages(void* args);
static void check_page_exists(char* page_data, char* page_title);

// getting pages info
static void get_page_links(PageData* page_data);
char* get_page_content(char* page_title);
static void parse_links(cJSON* json_data, PageData* page_data);
static void get_links_data(PageData* page_data);
static void make_links_data_req(PageData* page_data, char* curr_titles);

// freeing data
static void free_page_data(PageData* page_data);

// logic of trace
void* run_trace(void* args);

#endif

