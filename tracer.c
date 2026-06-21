/**
 * Handles the data recieved from Wikipedia. Stores the destination page data and scores
 * of each page fetched.
 *
 * @author Garret Wilson
 */


#include "tracer.h"


/*
 * Give the trace access to destination page title and content.
 *
 * @param dest_page: struct containing page title and content
 */
void set_dest_page(PageData* dest_page) {
  // TODO
}


/*
 * Checks to see if the a page's links contain the destination page.
 *
 * @param page_data: struct with current pages title and links
 */
void evaluate_page(PageData* page_data) {
  // TODO
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


