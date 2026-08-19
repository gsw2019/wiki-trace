/**
 * Prototypes, structs, and macros for for fetching Wikipedia data
 *
 * @author Garret Wilson
 */


#ifndef FETCHER_H
#define FETCHER_H

#include <curl/curl.h>

#include "tracer.h"
#include "view.h"
#include "utils/cJSON.h"


#define URL_LEN              500


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
  char* data;
  size_t size;
} Response;


extern TraceData trace_data;    // global var used to track trace state across view.c and fetcher.c


// init or utility funcs
void init_curl();
static size_t write_callback(void *ptr, size_t size, size_t nmemb, Response *res);

// verifying pages input by user
void verify_pages();
static void check_page_exists(char* page_data, char* page_title);

// getting pages links
void get_page_links(PageData* curr_page);
static void parse_links(cJSON* json_data, PageData* curr_page);

// getting pages links data
void get_links_data(PageData* curr_page);
static void make_links_data_req(char* curr_titles, PageData* curr_page);
static void parse_links_data(cJSON* json_data, PageData* curr_page);

// getting page content
char* get_page_content(char* page_title);

// freeing data
static void free_page_data();

#endif  // FETCHER_H

