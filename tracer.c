/**
 * Handles the data recieved from Wikipedia. Stores the destination page data and scores
 * of each page fetched.
 *
 * @author Garret Wilson
 */


#include <pthread.h>
#include <string.h>
#include <stdlib.h>

#include "tracer.h"
#include "fetcher.h"


DestPage destination_page;


FILE* file;


/*
 * Give the tracer its own struct that contains the destination page info 
 *
 * @param dest_page: struct containing page title, intro, and content
 */
void set_dest_page(PageData* dest_page) {
  destination_page.title = strdup(dest_page->title);
  destination_page.intro = strdup(dest_page->intro);
  destination_page.content = strdup(dest_page->content);
}


/*
 * Checks to see if the a page's links contain the destination page.
 *
 * @param page_data: struct with current pages title and links
 */
void evaluate_page(PageData* page_data) {
  pthread_mutex_lock(&trace_data.lock);
  for (int i=0; i < page_data->num_links; i++) {

    // fprintf(file, "%s\n", page_data->links[i]);
    // fflush(file);

    if (strcmp(page_data->links[i], destination_page.title) == 0) {
      trace_data.trace_complete = 1;
      trace_data.trace_successful = 1;
      trace_data.num_pages_traveled++;

      // add dest page to list of traveled pages
      char** temp = realloc(trace_data.pages_traveled, trace_data.num_pages_traveled * sizeof(char*));
      if (temp == NULL) {
        trace_data.status = ERROR_MEM;
        trace_data.err_message = "System memory error.";
        pthread_mutex_unlock(&trace_data.lock);
        return;
      }
      trace_data.pages_traveled = temp;
      trace_data.pages_traveled[trace_data.num_pages_traveled-1] = strdup(destination_page.title);

      pthread_mutex_unlock(&trace_data.lock);
      return;
    }
  }
  pthread_mutex_unlock(&trace_data.lock);
}


/*
 * Performs scoring on the paage intros using the TF of their intros and the
 * TF of the destination pages inro
 *
 * @param page_data: struct with current pages links' titles and intros
 */
void score_intros(PageData* page_data) {
  // TODO
}


/*
 * Returns the page with the highest similarity score computed
 *
 * @return a page title
 */
char* get_next_page() {

  return "temp";
}


