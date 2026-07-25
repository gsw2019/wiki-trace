/**
 * Prototypes, structs, and macros for scoring and managing Wikipedia data
 *
 * @author Garret Wilson
 */


#ifndef TRACER_H
#define TRACER_H


#include "fetcher.h"
#include "hash_table.h"


#define DELIMITERS " \n\t\r"


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

// func check if trace is done
void evaluate_page(PageData* page_data);

// func to choose next page
void score_intros(PageData* page_data);
char* get_next_page();


#endif
