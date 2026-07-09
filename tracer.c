/**
 * Handles the data recieved from Wikipedia. Stores the destination page data and scores
 * of each pages fetched.
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

HashTable ht_stopwords;

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
char* clean_term(char* token) {
  char* dest = token;
  char* src = token;

  while (*src) {
    // skip char
    // ctype.h functions expect unsigned char
    if (ispunct((unsigned char) *src)) {
      src++;
    }

    // make lower and append
    else if (isupper((unsigned char) *src)) {
      *dest = tolower((unsigned char) *src);
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

  return token;
}


/*
 * Computes the destination pages content term frequencies
 *
 * @param string: the string to compute frequencies for
 * @return a hash table of term frequencies
 */
HashTable* compute_term_freq(char* string) {
  HashTable* ht = calloc(1, sizeof(HashTable));
  if (ht == NULL) { return NULL; } 

  // make mutable array for tokenization
  char str_arr[strlen(string) + 1];
  strcpy(str_arr, string);

  // loop through terms
  char* token = strtok(str_arr, DELIMITERS);
  while (token) {
    // clean term
    char* clean_token = clean_term(token);

    // empty string
    if (!(*clean_token)) {
      token = strtok(NULL, DELIMITERS);
      continue;
    }

    // stem word
    char stemmed_token[strlen(clean_token) + 1];
    strcpy(stemmed_token, clean_token);
    int end = stem(clean_token, 0, strlen(clean_token) - 1);
    stemmed_token[end + 1] = '\0';

    // continue if a stop word
    if (hash_table_get(&ht_stopwords, stemmed_token) != -1) {
      token = strtok(NULL, DELIMITERS);
      continue;
    }

    // not in hash table
    // add it
    else if (hash_table_get(ht, stemmed_token) == -1) {
      hash_table_add(ht,stemmed_token, 1);
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
 * Reads stop words into a hash table for quick lookup. Avoids opening and closing a
 * file for each evaluation of a word.
 */
void init_stopwords() {
  stop_words_file = fopen("EN_stopwords.txt", "r");
  if (stop_words_file == NULL) {
    pthread_mutex_lock(&trace_data.lock);
    trace_data.trace_complete = 1;
    trace_data.status = ERROR_FILE;
    trace_data.err_message = "Opening file error. Reading EN stop words";
    pthread_mutex_unlock(&trace_data.lock);
    return;
  }

  char* stop_word = NULL;
  size_t len = 0;
  while (getline(&stop_word, &len, stop_words_file) != -1) {
    // string complentary span
    // return number of chars before first char in second param
    // since using as index, willl make that char a null terminator
    stop_word[strcspn(stop_word, DELIMITERS)] = '\0';

    // add word if not in table
    if (hash_table_get(&ht_stopwords, stop_word) == -1) {
      hash_table_add(&ht_stopwords, stop_word, 1.0);
    }
  }

  free(stop_word);
  fclose(stop_words_file);
}


/*
 * Set destination page content
 *
 * @param page_content: content of page which is the destination page
 */
void set_dest_page_content(char* page_content) {
  int status;

  destination_page.content = strdup(page_content);

  init_stopwords();
  pthread_mutex_lock(&trace_data.lock);
  status = trace_data.status;
  pthread_mutex_unlock(&trace_data.lock);
  if (status) { return; }

  destination_page.content_tf = compute_term_freq(destination_page.content);
  if (destination_page.content_tf == NULL) {
    pthread_mutex_lock(&trace_data.lock);
    trace_data.trace_complete = 1;
    trace_data.status = ERROR_MEM;
    trace_data.err_message = "System memory error. Destination page tf hash table initialization."; 
    pthread_mutex_unlock(&trace_data.lock);
    return;
  }

  hash_table_print(destination_page.content_tf, file);
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
        trace_data.err_message = "System memory error. Adding destination page to pages traveled";
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

