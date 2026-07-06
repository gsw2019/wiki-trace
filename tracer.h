/**
 * Prototypes, structs, and macros for scoring and managing Wikipedia data
 *
 * @author Garret Wilson
 */


#ifndef TRACER_H
#define TRACER_H


#include "fetcher.h"


typedef struct {
  char* title;
  char* intro;
  char* content;
} DestPage;


void set_dest_page(PageData* dest_page);
void evaluate_page(PageData* page_data);
void score_intros(PageData* page_data);
char* get_next_page();


#endif
