/**
 * Handles the data recieved from Wikipedia. Stores the destination page data and scores
 * of each page fetched.
 *
 * @author Garret Wilson
 */


#include <pthread.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>

#include "tracer.h"
#include "fetcher.h"
#include "hash_table.h"
#include "stmr.h"


DestPage destination_page;

FILE* stop_words_file;

FILE* file;


/*
 * Set destination page title
 *
 * @param page_title: title of page which is the destination page
 */
void set_dest_page_title(char* page_title) {
  file = fopen("output.txt", "w");

  destination_page.title = strdup(page_title); 
}


/*
 * Cleans a term such that it is all lower case and has all punctuation removed
 *
 * @param token: term to be cleaned
 */
void clean_term(char* token) {
  char* dest = token;
  char* src = token;

  while (*src) {
    // skip char
    if (ispunct(*src)) {
      src++;
    }

    // make lower and append
    else if (isupper(*src)) {
      *dest = tolower(*src);
      src++;
      dest++;
    }

    // equal chars
    else {
      *dest = *src;
      src++;
      dest++;
    }
  }

  *dest = '\0';
}


/*
 * checks if a word is a stop word or not
 *
 * @param token: term to be checked for being a stop word
 * @return 0 if token is not a stop word, 1 otherwise
 */
int is_stop_word(char* token) {
  stop_words_file = fopen("EN_stopwords.txt", "r");

  char* stop_word = NULL;
  size_t len = 0;
  while (getline(&stop_word, &len, stop_words_file) != -1) {
    // string complentary span
    // return number of chars before first char in second param
    // since using as index, willl make that char a null terminator
    stop_word[strcspn(stop_word, DELIMITERS)] = '\0';
    if (strcmp(token, stop_word) == 0) { 
      free(stop_word);
      fclose(stop_words_file);
      return 1; 
    }
  }

  free(stop_word);
  fclose(stop_words_file);

  return 0;
}


/*
 * Computes the destination pages content term frequencies
 *
 * @param string: the string to compute frequencies for
 * @return a hash table of term frequencies
 */
HashTable* compute_term_freq(char* string) {
  HashTable* ht = malloc(sizeof(HashTable));

  // make mutable array for tokenization
  char str_arr[strlen(string) + 1];
  strcpy(str_arr, string);

  // loop through terms
  char* token = strtok(str_arr, DELIMITERS);
  while (token) {
    // clean term
    clean_term(token);

    // stem word
    char stemmed_token[strlen(token) + 1];
    strcpy(stemmed_token, token);
    int end = stem(token, 0, strlen(token) - 1);
    stemmed_token[end + 1] = '\0';

    // continue if a stop word
    if (is_stop_word(stemmed_token)) {
      token = strtok(NULL, DELIMITERS);
      continue;
    }

    // not in hash table
    // add it
    else if (hash_table_get(ht, stemmed_token) == -1) {
      hash_table_add(ht, stemmed_token, 1);
    }

    // in hash table, update frequency
    else {
      double old_val = hash_table_get(ht, stemmed_token);
      hash_table_set(ht, stemmed_token, ++old_val);
    }

    token = strtok(NULL, DELIMITERS);
  }

  return ht;

}


/*
 * Set destination page content
 *
 * @param page_content: content of page which is the destination page
 */
void set_dest_page_content(char* page_content) { 
  destination_page.content = strdup(page_content);
  destination_page.content_tf = compute_term_freq(destination_page.content);
}


/*
 * Checks to see if a page's links contain the destination page.
 *
 * @param page_data: struct with current pages title and links
 */
void evaluate_page(PageData* page_data) {
  pthread_mutex_lock(&trace_data.lock);

  for (int i=0; i < page_data->num_links; i++) {
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
      trace_data.pages_traveled[trace_data.num_pages_traveled - 1] = strdup(destination_page.title);

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


