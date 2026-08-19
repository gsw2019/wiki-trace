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
#include <math.h>

#include "tracer.h"
#include "fetcher.h"
#include "logger.h"
#include "utils/cJSON.h"
#include "utils/hash_table.h"
#include "utils/stmr.h"


DestPage destination_page;

PageData curr_page;

HashTable ht_stopwords;

char** pages_visited;

char* next_page;


FILE* file;


/*
 * Cleans a term such that it is all lower case and has all punctuation removed
 *
 * @param token: term to be cleaned
 */
char* clean_term(char* token)
{
  char* dest = token;
  char* src = token;

  while (*src)
  {
    // skip char
    // ctype.h functions expect unsigned char
    if (ispunct((unsigned char) *src)) { src++; }

    // make lower and append
    else if (isupper((unsigned char) *src))
    {
      *dest = tolower((unsigned char) *src);
      src++;
      dest++;
    }

    // equal chars
    else
    {
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
HashTable* compute_term_freq(char* string) 
{
  HashTable* ht = calloc(1, sizeof(HashTable));
  if (ht == NULL) { LOG_ERROR(ERROR_CALLOC, NULL, NULL); }

  // make mutable array for tokenization
  char str_arr[strlen(string) + 1];
  strcpy(str_arr, string);

  // loop through terms
  char* token = strtok(str_arr, DELIMITERS);
  while (token)
  {
    // clean term
    char* clean_token = clean_term(token);

    // empty string
    if (!(*clean_token))
    {
      token = strtok(NULL, DELIMITERS);
      continue;
    }

    // stem word
    char stemmed_token[strlen(clean_token) + 1];
    strcpy(stemmed_token, clean_token);
    int end = stem(clean_token, 0, strlen(clean_token) - 1);
    stemmed_token[end + 1] = '\0';

    // continue if a stop word
    if (hash_table_get(&ht_stopwords, stemmed_token) != -1)
    {
      token = strtok(NULL, DELIMITERS);
      continue;
    }

    // not in hash table
    // add it
    else if (hash_table_get(ht, stemmed_token) == -1) { hash_table_add(ht,stemmed_token, 1); }

    // in hash table, update frequency
    else
    {
      double old_val = hash_table_get(ht, stemmed_token);
      hash_table_set(ht, stemmed_token, ++old_val);
    }

    token = strtok(NULL, DELIMITERS);
  }

  // iterate over all hash table entries and log10() their tf
  for (int i = 0; i < NUM_BUCKETS; i++)
  {
    if (ht->buckets[i]) 
    {
      Node* node = ht->buckets[i];
      while (node) 
      {
        node->value = log10(node->value);
        node = node->next;
      }
    }
  }

  return ht;
}


/*
 * Reads stop words into a hash table for quick lookup. Avoids opening and closing a
 * file for each evaluation of a word.
 */
void init_stopwords()
{
  FILE* stop_words_file = fopen(STOPWORDS_FILE,"r");
  if (stop_words_file == NULL)
  {
    LOG_ERROR(ERROR_FILE, NULL, STOPWORDS_FILE);
    return;
  }

  char* stop_word = NULL;
  size_t len = 0;
  while (getline(&stop_word, &len, stop_words_file) != -1)
  {
    // string complentary span
    // return number of chars before first char in second param
    // since using as index, willl make that char a null terminator
    stop_word[strcspn(stop_word, DELIMITERS)] = '\0';

    // add word if not in table
    if (hash_table_get(&ht_stopwords, stop_word) == -1)
    {
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
void set_dest_page_content(char* page_content)
{
  int status;

  destination_page.content = strdup(page_content);

  init_stopwords();
  pthread_mutex_lock(&trace_data.lock);
  status = trace_data.status;
  pthread_mutex_unlock(&trace_data.lock);
  if (status) { return; }

  destination_page.content_tf = compute_term_freq(destination_page.content);
  if (destination_page.content_tf == NULL) { return; }
}


/*
 * Updates the global shared struct that tracks the pages traveled
 *
 * @param page_title: the next page in the trace
 */
void update_pages_traveled(char* page_title)
{
  pthread_mutex_lock(&trace_data.lock);

  if (trace_data.pages_traveled_size >= trace_data.pages_traveled_capacity) {
    trace_data.pages_traveled_capacity *= 2;

    char** temp = realloc(trace_data.pages_traveled, sizeof(char*) * trace_data.pages_traveled_capacity);
    if (temp == NULL) { LOG_ERROR(ERROR_REALLOC, NULL, NULL); }

    trace_data.pages_traveled = temp;
  }

  trace_data.pages_traveled[trace_data.pages_traveled_size] = strdup(page_title);
  trace_data.pages_traveled_size++;

  pthread_mutex_unlock(&trace_data.lock);
}


/*
 * Performs scoring on the page links using the TF of their intros and the TF of the
 * destination pages intro.
 *
 * @param page_data: struct with current pages links' titles and intros
 */
void score_intros(PageData* page_data)
{
  /* fprintf(file, "in score_intros\n"); */
  /* fprintf(file, "num page links: %d\n", page_data->num_links); */
  /* fprintf(file, "num links data: %d\n", page_data->num_links_data); */
  /* fflush(file); */
  /**/

  double top_score = 0;
  char* top_title;

  // iterate over all intro objects
  for (int i=0; i < page_data->links_data_size; i++)
  {
    double curr_score = 0;

    // walk JSON to get intro text
    cJSON* curr = page_data->links_data[i];
    cJSON* curr_intro_obj = cJSON_GetObjectItem(curr, "extract");
    char* curr_intro = curr_intro_obj->valuestring;

    // get a tf of extract
    HashTable* curr_tf_ht = compute_term_freq(curr_intro);

    // iterate over curr hash table and sum the tf of tokens shared with destination page
    for (int j=0; j < NUM_BUCKETS; j++)
    {
      if (curr_tf_ht->buckets[j] != NULL)
      {
        Node* node = curr_tf_ht->buckets[j];
        while (node)
        {
          // check if in destination page
          double dest_page_val = hash_table_get(destination_page.content_tf, node->key);
          if ( dest_page_val != -1) { curr_score += node->value + dest_page_val; }
          node = node->next;
        }
      }
    }

    if (curr_score > top_score)
    {
      top_score = curr_score;

      // get title from JSON object
      cJSON* curr_title_obj = cJSON_GetObjectItem(curr, "title");
      top_title = strdup(curr_title_obj->valuestring);
    }
  }

  /* fprintf(file, "%s\n", top_title); */
  /* fflush(file); */

  next_page = strdup(top_title);
}


/*
 * Returns the page with the highest similarity score computed
 *
 * @return a page title
 */
char* get_next_page() { return next_page; }


/*
 * Checks to see if a page's links contain the destination page.
 *
 * @param page_data: struct with current pages title and links
 */
void evaluate_page(PageData* page_data)
{
  for (int i=0; i < page_data->links_titles_size; i++)
  {
    if (strcmp(page_data->links_titles[i], destination_page.title) == 0)
    {
      pthread_mutex_lock(&trace_data.lock);

      trace_data.trace_complete = 1;
      trace_data.trace_successful = 1;

      // add dest page to list of traveled pages
      if (trace_data.pages_traveled_size >= trace_data.pages_traveled_capacity)
      {
        trace_data.pages_traveled_capacity *= 2;

        char** temp = realloc(trace_data.pages_traveled, sizeof(char*) * trace_data.pages_traveled_capacity);
        if (temp == NULL) { LOG_ERROR(ERROR_REALLOC, NULL, NULL); }
        trace_data.pages_traveled = temp;
      }

      trace_data.pages_traveled[trace_data.pages_traveled_size] = strdup(destination_page.title);
      trace_data.pages_traveled_size++;

      pthread_mutex_unlock(&trace_data.lock);

      return;
    }
  }
}


/*
 * Cleans up the PageData stuct used by the current page. Frees its links and intros fields.
 * Readies it for use on next iteration.
 */
static void free_page_data()
{
  // free array of link titles
  for (int i=0; i < curr_page.links_titles_size; i++) { free(curr_page.links_titles[i]); }
  free(curr_page.links_titles);

  // free array of cJSON objects
  for (int i=0; i < curr_page.links_data_size; i++) { free(curr_page.links_data[i]); }
  free(curr_page.links_data);
}


/*
 * Begins running the trace by fetching page links and intros. Ran by a worker thread.
 *
 * @param args: NULL, using global state var trace_data in worker
 */
void* run_trace(void* args)
{
  pthread_mutex_lock(&trace_data.lock);
  char* start_page_title = strdup(trace_data.start_page);
  char* dest_page_title = strdup(trace_data.dest_page);
  trace_data.pages_traveled = malloc(trace_data.pages_traveled_capacity * sizeof(char*));
  char** temp = trace_data.pages_traveled;
  trace_data.pages_traveled[0] = strdup(trace_data.start_page);
  trace_data.pages_traveled_size++;
  trace_data.trace_in_progress = 1;
  pthread_mutex_unlock(&trace_data.lock);

  if (temp == NULL) { LOG_ERROR(ERROR_MALLOC, NULL, NULL); }

  int trace_complete;

  // set curr page title
  curr_page.title = start_page_title;

  // get start page links
  get_page_links(&curr_page);
  pthread_mutex_lock(&trace_data.lock);
  int status = trace_data.status;
  pthread_mutex_unlock(&trace_data.lock);
  if (status != 0) { return NULL; }

  // initialize tracer
  init_tracer(dest_page_title);

  // evaluate curr page for dest page title
  evaluate_page(&curr_page);

  // check if dest page was on start page or error and end if either
  pthread_mutex_lock(&trace_data.lock);
  trace_complete = trace_data.trace_complete;
  status = trace_data.status;
  pthread_mutex_unlock(&trace_data.lock);
  if (trace_complete == 1 || status != 0) { return NULL; }

  set_dest_page_content(get_page_content(dest_page_title));

  // leave before looping if encountered errors
  pthread_mutex_lock(&trace_data.lock);
  status = trace_data.status;
  pthread_mutex_unlock(&trace_data.lock);
  if (status != 0) { return NULL; }

  // while destination page not found
  int iter = 1;
  while (trace_complete == 0) {

    get_links_data(&curr_page);
    pthread_mutex_lock(&trace_data.lock);
    status = trace_data.status;
    pthread_mutex_unlock(&trace_data.lock);
    if (status != 0) { return NULL; }

    /* for (int i = 0; i < curr_page.num_links; i++) { */
    /*   fprintf(file, "%s\n\n", curr_page.links_data[i]); */
    /* } */
    /* fflush(file); */

    //
    // functioning above here
    //

    score_intros(&curr_page);

    char* next_page = get_next_page();

    update_pages_traveled(next_page);

    free_page_data();

    curr_page.title = next_page;

    get_page_links(&curr_page);

    evaluate_page(&curr_page);

    pthread_mutex_lock(&trace_data.lock);
    trace_complete = trace_data.trace_complete;
    pthread_mutex_unlock(&trace_data.lock);
  }

  return NULL;
}


/*
 * Set up things the tracer will have to utilize
 */
void init_tracer(char* page_title)
{
  file = fopen("output.txt", "a");
  if (file == NULL) {
    perror("fopen");
    exit(EXIT_FAILURE);
  }

  destination_page.title = strdup(page_title); 

  init_stopwords();

  pages_visited = malloc(INIT_ARRAY_SIZE * sizeof(char*));
}

