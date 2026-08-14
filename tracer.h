/**
 * Prototypes, structs, and macros for scoring and managing Wikipedia data
 *
 * @author Garret Wilson
 */


#ifndef TRACER_H
#define TRACER_H

#include "cJSON.h"
#include "hash_table.h"


#define DELIMITERS " \n\t\r"

typedef struct {
  char* title;
  char* content;
  char** links_titles;
  int links_titles_size;
  int links_titles_capacity;
  cJSON** links_data;
  int links_data_size;
  int links_data_capacity;
} PageData;

typedef struct {
  char* title;
  char* content;
  HashTable* content_tf;
} DestPage;


// initialize tracer
void init_tracer(char* page_title);

// funcs to set destination page info
void set_dest_page_content(char* page_content);

// funcs to clean terms and compute frequencies
char* clean_term(char* token);
HashTable* compute_term_freq(char* string);

// func to choose next page
void score_intros(PageData* page_data);
char* get_next_page();

// func to update global shared struct
void update_pages_traveled(char* page_title);

// func check if trace is done
void evaluate_page(PageData* page_data);

// run the trace
void* run_trace(void* args);

#endif
