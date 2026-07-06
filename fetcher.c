/**
 * Implementations for the fetcher.h prototypes. Handles requests for Wikipedia data via curl
 *
 * @author Garret Wilson
 */


#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "fetcher.h"
#include "tracer.h"
#include "cJSON.h"


CURL* curl;   // handle used in all fetching/crawling

URLParts url_parts;   // the url pieces needed for requests

FILE* file;


/*
 * Initialize curl and URL pieces required for our requests.
 *
 * @return a handle to curl object
 */
CURL* init_curl() {
  file = fopen("output.txt", "w");

  curl = curl_easy_init();
  curl_easy_setopt(curl, CURLOPT_USERAGENT, "wiki-trace project (wsg2026@outlook.com)");

  url_parts.verify_start = "https://en.wikipedia.org/w/api.php?action=query&format=json&prop=&titles=";
  url_parts.verify_end = "&formatversion=2";

  url_parts.links_start = "https://en.wikipedia.org/w/api.php?action=query&format=json&prop=links&titles=";
  url_parts.links_end = "&formatversion=2&pllimit=max";

  url_parts.links_cont_start = "https://en.wikipedia.org/w/api.php?action=query&format=json&prop=links&continue=%7C%7C&titles=";
  url_parts.links_cont_mid = "&formatversion=2&pllimit=max&plcontinue=";

  url_parts.intro_start = "https://en.wikipedia.org/w/api.php?action=query&format=json&prop=extracts&titles=";
  url_parts.intro_end = "&formatversion=2&exintro=1&explaintext=1";

  url_parts.content_start = "https://en.wikipedia.org/w/api.php?action=query&format=json&prop=extracts&titles=";
  url_parts.content_end = "&formatversion=2&explaintext=1";

  return curl;
}


/*
 * function used by curl to gather and concate the chunks it returns from the response
 *
 * @param ptr:   delivered data
 * @param size:  always 1 (fwrite relic, unused here)
 * @param nmemb: size of delivered data
 * @param res:   the pointer provided in the WRITEDATA setopt
 */
size_t write_callback(void *ptr, size_t size, size_t nmemb, Response *res) {
  size_t new_size = res->size + nmemb;    // increase size by adding size of retrieved data
  res->data = realloc(res->data, new_size + 1);
  memcpy(res->data + res->size, ptr, nmemb);    // pointer addition to get destination for new data
  res->size = new_size;
  res->data[res->size] = '\0';    // size as index, puts null term correctly

  return size * nmemb;
}


/*
 * Checks response data to see if the Wikipedia page exists
 *
 * @param page_data: string of json data from response
 * @return 0: page exists
 * @return ERROR_PARSE: jsom parse error
 * @return ERROR_EXISTENCE: page is missing
 */
static int check_page_exists(char* page_data) {
  cJSON* json_data = cJSON_Parse(page_data);
  const char *error_ptr = cJSON_GetErrorPtr();
  if (error_ptr != NULL) { 
    fprintf(stderr, "cJSON parse failed. Error before: %s\n", error_ptr);
    cJSON_Delete(json_data);
    return ERROR_PARSE;
  }

  // walk down JSON object
  cJSON* query = cJSON_GetObjectItem(json_data, "query");
  cJSON* data = cJSON_GetObjectItem(query, "pages");
  cJSON* first_page = cJSON_GetArrayItem(data, 0);
  cJSON* missing = cJSON_GetObjectItem(first_page, "missing");

  if (missing != NULL) {
    cJSON_Delete(data);
    return ERROR_EXISTENCE;
  }

  cJSON_Delete(data);

  return 0;
}


/*
 * Verify user input. Returns different error values depending on the error.
 *
 * @param args: NULL, using global state var trace_data in worker
 */
void* verify_pages(void* args) {
  // if missing one or both titles, return err value
  pthread_mutex_lock(&trace_data.lock);
  if (strlen(trace_data.start_page) == 0 || strlen(trace_data.dest_page) == 0) {
    trace_data.init_complete = 1;
    trace_data.status = ERROR_INPUT;
    trace_data.err_message = "Missing page titles. Please enter two Wikipedia pages.";
    pthread_mutex_unlock(&trace_data.lock);
    return NULL;
  }
  pthread_mutex_unlock(&trace_data.lock);

  // turn user input into URL style params
  pthread_mutex_lock(&trace_data.lock);
  char* start_page = curl_easy_escape(curl, trace_data.start_page, 0);
  char* dest_page = curl_easy_escape(curl, trace_data.dest_page, 0);
  pthread_mutex_unlock(&trace_data.lock);

  // build URLs
  char url_start_page[URL_LEN] = {0};
  strcat(url_start_page, url_parts.verify_start);
  strcat(url_start_page, start_page);
  strcat(url_start_page, url_parts.verify_end);

  char url_dest_page[URL_LEN] = {0};
  strcat(url_dest_page, url_parts.verify_start);
  strcat(url_dest_page, dest_page);
  strcat(url_dest_page, url_parts.verify_end);

  // make requests
  Response start_page_resp = { .data = malloc(1), .size = 0 };
  if (start_page_resp.data == NULL) {
    pthread_mutex_lock(&trace_data.lock);
    trace_data.init_complete = 1;
    trace_data.status = ERROR_MEM;
    trace_data.err_message = "System memory error.";
    pthread_mutex_unlock(&trace_data.lock);
    return NULL;
  }
  curl_easy_setopt(curl, CURLOPT_URL, url_start_page);
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &start_page_resp);
  curl_easy_perform(curl);

  Response dest_page_resp = { .data = malloc(1), .size = 0 };
  if (dest_page_resp.data == NULL) {
    pthread_mutex_lock(&trace_data.lock);
    trace_data.init_complete = 1;
    trace_data.status = ERROR_MEM;
    trace_data.err_message = "System memory error";
    pthread_mutex_unlock(&trace_data.lock);
    return NULL;
  }
  curl_easy_setopt(curl, CURLOPT_URL, url_dest_page);
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &dest_page_resp);
  curl_easy_perform(curl);

  int start_page_status = check_page_exists(start_page_resp.data);

  if (start_page_status == ERROR_PARSE) {
    pthread_mutex_lock(&trace_data.lock);
    trace_data.init_complete = 1;
    trace_data.status = ERROR_PARSE;
    trace_data.err_message = "Start page parse error. Quit to see error message.";
    pthread_mutex_unlock(&trace_data.lock);

    free(start_page_resp.data);
    free(dest_page_resp.data);

    return NULL;
  }
  else if (start_page_status == ERROR_EXISTENCE) {
    pthread_mutex_lock(&trace_data.lock);
    trace_data.init_complete = 1;
    trace_data.status = ERROR_EXISTENCE;
    trace_data.err_message = "Start page does not exists. Make sure to enter the page title\n"
                              "exactly as it appears on Wikipedia. Copy and paste is acceptable.";
    pthread_mutex_unlock(&trace_data.lock);

    free(start_page_resp.data);
    free(dest_page_resp.data);

    return NULL;
  }

  int dest_page_status = check_page_exists(dest_page_resp.data);

  if (dest_page_status == ERROR_PARSE) {
    pthread_mutex_lock(&trace_data.lock);
    trace_data.init_complete = 1;
    trace_data.status = ERROR_PARSE;
    trace_data.err_message = "Dest page parse error. Quit to see error message.";
    pthread_mutex_unlock(&trace_data.lock);

    free(start_page_resp.data);
    free(dest_page_resp.data);

    return NULL;
  }
  else if (dest_page_status == ERROR_EXISTENCE) {
    pthread_mutex_lock(&trace_data.lock);
    trace_data.init_complete = 1;
    trace_data.status = ERROR_EXISTENCE;
    trace_data.err_message = "Dest page does not exists. Make sure to enter the page title\n"
                              "exactly as it appears on Wikipedia. Copy and paste is acceptable.";
    pthread_mutex_unlock(&trace_data.lock);

    free(start_page_resp.data);
    free(dest_page_resp.data);

    return NULL;
  }

  free(start_page_resp.data);
  free(dest_page_resp.data);

  pthread_mutex_lock(&trace_data.lock);
  trace_data.init_complete = 1;
  trace_data.status = 0;
  pthread_mutex_unlock(&trace_data.lock);

  return NULL;
}


/*
 * Gets the link titles out of the JSON data and stores them in PageData struct
 *
 * @param json_data: request response
 * @param page_data: struct to write links to
 */
static void parse_links(cJSON* json_data, PageData* page_data) {
  cJSON* query = cJSON_GetObjectItem(json_data,"query");
  cJSON* pages = cJSON_GetObjectItem(query, "pages");
  cJSON* pages_element = cJSON_GetArrayItem(pages, 0);
  cJSON* links_array = cJSON_GetObjectItem(pages_element, "links");

  int capacity = INIT_DATA_ARRAY_SIZE;    // num of space initialized array has
  int count = page_data->num_links;    // how many elements we have added
  int num_links = cJSON_GetArraySize(links_array);

  // dynamically write to page data links field
  for (int i=0; i < num_links; i++) {
    // increase size if we dont have room
    if (count >= capacity) {
      capacity = count + (capacity * 2);
      char** temp = realloc(page_data->links, capacity * sizeof(char*));
      if (temp == NULL) {
        pthread_mutex_lock(&trace_data.lock);
        trace_data.trace_complete = 1;
        trace_data.status = ERROR_MEM;
        trace_data.err_message = "System memory error.";
        pthread_mutex_unlock(&trace_data.lock);
        return;
      }

      page_data->links = temp;
    }

    // allocate mem for individual title
    cJSON* links_element = cJSON_GetArrayItem(links_array, i);

    //
    // can add conditional for only ns:0, only main articles
    //

    cJSON* link = cJSON_GetObjectItem(links_element, "title");
    page_data->links[count] = malloc(strlen(link->valuestring) + 1);
    if (page_data->links[count] == NULL) {
      pthread_mutex_lock(&trace_data.lock);
      trace_data.trace_complete = 1;
      trace_data.status = ERROR_MEM;
      trace_data.err_message = "System memory error.";
      pthread_mutex_unlock(&trace_data.lock);
      return;
    }

    // copy string into allocated mem
    strcpy(page_data->links[count], link->valuestring);

    /* fprintf(file, "%s\n", link->valuestring); */

    count++;
  }

  page_data->num_links = count;
}


/*
 * Gets all of the specified pages links
 *
 * @param page_title: title of page to get links of
 * @param curr_page: struct to write page data to
 */
static void get_page_links(char* page_title, PageData* page_data) {
  // initialize struct to hold response data
  Response page_response = { .data = malloc(1), .size = 0 };
  if (page_response.data == NULL) {
    pthread_mutex_lock(&trace_data.lock);
    trace_data.trace_complete = 1;
    trace_data.status = ERROR_MEM;
    trace_data.err_message = "System memory error.";
    pthread_mutex_unlock(&trace_data.lock);
    return;
  }

  // initialize struct to hold page data
  page_data->title = page_title;
  page_data->links = malloc(INIT_DATA_ARRAY_SIZE * sizeof(char*));
  page_data->num_links = 0;
  if (page_data->links == NULL) {
    pthread_mutex_lock(&trace_data.lock);
    trace_data.trace_complete = 1;
    trace_data.status = ERROR_MEM;
    trace_data.err_message = "System memory error.";
    pthread_mutex_unlock(&trace_data.lock);
    return;
  }

  // build URL for page links
  char url_page_links[URL_LEN] = {0};
  char* url_page_title = curl_easy_escape(curl, page_title, 0);
  strcat(url_page_links, url_parts.links_start);
  strcat(url_page_links, url_page_title);
  strcat(url_page_links, url_parts.links_end);

  // request all links on page
  curl_easy_setopt(curl, CURLOPT_URL, url_page_links);
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &page_response);
  curl_easy_perform(curl);

  // check for parse error
  cJSON* json_data = cJSON_Parse(page_response.data);
  const char *error_ptr = cJSON_GetErrorPtr();
  if (error_ptr != NULL) {
    fprintf(stderr, "cJSON parse failed. Error before: %s\n", error_ptr);
    pthread_mutex_lock(&trace_data.lock);
    trace_data.trace_complete = 1;
    trace_data.status = ERROR_PARSE;
    trace_data.err_message = "JSON parse error. Quit to see error message.";
    pthread_mutex_unlock(&trace_data.lock);
    cJSON_Delete(json_data);
    return;
  }

  // extract link titles
  parse_links(json_data, page_data);

  // leave if encountered errors
  int err;
  pthread_mutex_lock(&trace_data.lock);
  err = trace_data.status;
  pthread_mutex_unlock(&trace_data.lock);
  if (err != 0) { return; }

  // check if has continue flag
  if (cJSON_GetObjectItem(json_data, "continue") != NULL) {

    int cont_request = 1;
    cJSON *cont, *cont_string_obj;

    while (cont_request) {
      // get continue data
      cont = cJSON_GetObjectItem(json_data, "continue");
      cont_string_obj = cJSON_GetObjectItem(cont, "plcontinue");
      char* cont_string = cont_string_obj->valuestring;

      // clear response for next request
      free(page_response.data);
      page_response.data = malloc(1);
      page_response.size = 0;

      // build continue URL
      char continue_url[URL_LEN] = {0};
      char* url_cont_string = curl_easy_escape(curl, cont_string, 0);
      strcat(continue_url, url_parts.links_cont_start);
      strcat(continue_url, url_page_title);
      strcat(continue_url, url_parts.links_cont_mid);
      strcat(continue_url, url_cont_string);

      // make request
      curl_easy_setopt(curl, CURLOPT_URL, continue_url);
      curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
      curl_easy_setopt(curl, CURLOPT_WRITEDATA, &page_response);
      curl_easy_perform(curl);

      // check for parse error
      json_data = cJSON_Parse(page_response.data);
      const char *error_ptr = cJSON_GetErrorPtr();
      if (error_ptr != NULL) {
        fprintf(stderr, "cJSON parse failed. Error before: %s\n", error_ptr);
        pthread_mutex_lock(&trace_data.lock);
        trace_data.trace_complete = 1;
        trace_data.status = ERROR_PARSE;
        trace_data.err_message = "JSON parse error. Quit to see error message.";
        pthread_mutex_unlock(&trace_data.lock);
        cJSON_Delete(json_data);
        return;
      }

      // extract links
      parse_links(json_data, page_data);

      // leave if encountered errors
      int err;
      pthread_mutex_lock(&trace_data.lock);
      err = trace_data.status;
      pthread_mutex_unlock(&trace_data.lock);
      if (err != 0) { return; }

      // check if has continue flag
      cont = cJSON_GetObjectItem(json_data, "continue");
      if (cont != NULL) { continue; }

      cont_request = 0;
    }
  }
}


/*
 * Gets the pages entire content.
 *
 * @param page_title: title of page to get content of
 * @param page: struct to write page content to
 */
static void get_page_content(char* page_title, PageData* page_data) {
  // initialize struct to hold response data
  Response page_response = { .data = malloc(1), .size = 0 };
  if (page_response.data == NULL) {
    pthread_mutex_lock(&trace_data.lock);
    trace_data.trace_complete = 1;
    trace_data.status = ERROR_MEM;
    trace_data.err_message = "System memory error.";
    pthread_mutex_unlock(&trace_data.lock);
    return;
  }

  // build URL for page content
  char url_page_content[URL_LEN] = {0};
  char* url_page_title = curl_easy_escape(curl, page_title, 0);
  strcat(url_page_content, url_parts.content_start);
  strcat(url_page_content, url_page_title);
  strcat(url_page_content, url_parts.content_end);

  // request page content
  curl_easy_setopt(curl, CURLOPT_URL, url_page_content);
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &page_response);
  curl_easy_perform(curl);

  // check for parse error
  cJSON* json_data = cJSON_Parse(page_response.data);
  const char *error_ptr = cJSON_GetErrorPtr();
  if (error_ptr != NULL) {
    fprintf(stderr, "cJSON parse failed. Error before: %s\n", error_ptr);
    pthread_mutex_lock(&trace_data.lock);
    trace_data.trace_complete = 1;
    trace_data.status = ERROR_PARSE;
    trace_data.err_message = "JSON parse error. Quit to see error message.";
    pthread_mutex_unlock(&trace_data.lock);
    cJSON_Delete(json_data);
    return;
  }



}


/*
 * Gets the intros for pages.
 *
 * @param page_data: struct containing links to get intros for
 */
static void get_links_intros(PageData* page_data) {
  // TODO
}


/*
 * Cleans up the PageData stuct used by the current page. Frees its links and intros fields.
 * Readies it for use on next iteration.
 *
 * @param page_data: struct need to clean
 */
static void free_page_data(PageData* page_data) {
  // TODO
}


/*
 * Begins running the trace by fetching page links and intros. Ran by a worker thread.
 *
 * @param args: NULL, using global state var trace_data in worker
 */
void* run_trace(void* args) {
  pthread_mutex_lock(&trace_data.lock);
  char* start_page_title = strdup(trace_data.start_page);
  char* dest_page_title = strdup(trace_data.dest_page);
  trace_data.pages_traveled = malloc(INIT_DATA_ARRAY_SIZE * sizeof(char*));
  trace_data.pages_traveled[0] = strdup(trace_data.start_page);
  trace_data.num_pages_traveled++;
  pthread_mutex_unlock(&trace_data.lock);

  PageData curr_page;
  PageData dest_page;

  // get start page links
  get_page_links(start_page_title, &curr_page);

  // evaluate curr page for dest page title
  evaluate_page(&curr_page);

  // check if dest on start and end if so
  int trace_complete;
  pthread_mutex_lock(&trace_data.lock);
  trace_complete = trace_data.trace_complete;
  pthread_mutex_unlock(&trace_data.lock);
  if (trace_complete) { return NULL; }

  // get dest page content
  get_page_content(dest_page_title, &dest_page);

  // leave if encountered errors
  int status;
  pthread_mutex_lock(&trace_data.lock);
  status = trace_data.status;
  pthread_mutex_unlock(&trace_data.lock);
  if (status != 0) { return NULL; }

  return NULL;

  // set destination info in tracer module
  set_dest_page(&dest_page);
 
  // while destination page not found
  while (trace_complete == 0) {
    get_links_intros(&curr_page);

    score_intros(&curr_page);

    char* next_page = get_next_page();

    trace_data.num_pages_traveled++;

    free_page_data(&curr_page);

    get_page_links(next_page, &curr_page);

    evaluate_page(&curr_page);

    pthread_mutex_lock(&trace_data.lock);
    trace_complete = trace_data.trace_complete;
    pthread_mutex_unlock(&trace_data.lock);

  }

  return NULL;
}


