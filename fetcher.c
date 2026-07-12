/**
 * Handles requests for Wikipedia data via curl. Organizes data for the page currently
 * being walked and sends it to tracer module. Sends destination paage data to tracer
 * module.
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

FILE* error_file;

FILE* file;


/*
 * Initialize curl and URL pieces required for our requests.
 *
 * @return a handle to curl object
 */
CURL* init_curl() {
  file = fopen("output.txt", "w");

  error_file = fopen("error_output.txt", "w");

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
    fprintf(error_file, "cJSON parse failed. Verify page parse error. Error before: %s\n", error_ptr);
    fflush(error_file);
    cJSON_Delete(json_data);
    return ERROR_PARSE;
  }

  // walk down JSON object
  cJSON* query = cJSON_GetObjectItem(json_data, "query");
  cJSON* data = cJSON_GetObjectItem(query, "pages");
  cJSON* first_page = cJSON_GetArrayItem(data, 0);
  cJSON* missing = cJSON_GetObjectItem(first_page, "missing");

  if (missing != NULL) {
    cJSON_Delete(json_data);
    return ERROR_EXISTENCE;
  }

  cJSON_Delete(json_data);

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
    trace_data.err_message = "System memory error.\n"
                             "Mallocing data field in Response struct for start page.";
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
    trace_data.err_message = "System memory error.\n"
                             "Mallocing data field in Response struct for destination page.";
    pthread_mutex_unlock(&trace_data.lock);
    return NULL;
  }
  curl_easy_setopt(curl, CURLOPT_URL, url_dest_page);
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &dest_page_resp);
  curl_easy_perform(curl);

  // check start page status
  int start_page_status = check_page_exists(start_page_resp.data);

  if (start_page_status == ERROR_PARSE) {
    pthread_mutex_lock(&trace_data.lock);
    trace_data.init_complete = 1;
    trace_data.status = ERROR_PARSE;
    trace_data.err_message = "Start page parse error. Check error file to see error message.";
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

  // check dest page status
  int dest_page_status = check_page_exists(dest_page_resp.data);

  if (dest_page_status == ERROR_PARSE) {
    pthread_mutex_lock(&trace_data.lock);
    trace_data.init_complete = 1;
    trace_data.status = ERROR_PARSE;
    trace_data.err_message = "Destination page parse error. Check error file to see error\n"
                             "message.";
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

  int capacity = page_data->capacity_links;    // num of space initialized array has
  int count = page_data->num_links;    // how many elements we have added
  int num_links = cJSON_GetArraySize(links_array);

  // dynamically write to page data links field
  for (int i=0; i < num_links; i++) {
    // increase size if we dont have room
    if (count >= capacity) {
      capacity *= 2;
      char** temp = realloc(page_data->links_titles, capacity * sizeof(char*));
      if (temp == NULL) {
        pthread_mutex_lock(&trace_data.lock);
        trace_data.trace_complete = 1;
        trace_data.status = ERROR_MEM;
        trace_data.err_message = "System memory error.\n"
                                 "Reallocing memory for page link titles array in PageData struct";
        pthread_mutex_unlock(&trace_data.lock);
        return;
      }

      page_data->capacity_links = capacity;
      page_data->links_titles = temp;
    }

    // get array element with link info
    cJSON* links_element = cJSON_GetArrayItem(links_array, i);

    //
    // can add conditional for only ns:0, only main articles
    //

    // allocate mem for individual title
    cJSON* link = cJSON_GetObjectItem(links_element, "title");
    page_data->links_titles[count] = malloc(strlen(link->valuestring) + 1);
    if (page_data->links_titles[count] == NULL) {
      pthread_mutex_lock(&trace_data.lock);
      trace_data.trace_complete = 1;
      trace_data.status = ERROR_MEM;
      trace_data.err_message = "System memory error.\n"
                               "Mallocing memory for a page link title element in PageData struct";
      pthread_mutex_unlock(&trace_data.lock);
      return;
    }

    // copy string into allocated mem
    strcpy(page_data->links_titles[count], link->valuestring);

    /* fprintf(file, "%s\n", link->valuestring); */

    count++;
  }

  page_data->num_links = count;
}


/*
 * Gets all of the specified page's links
 *
 * @param page_title: title of page to get links of
 * @param curr_page: struct to write page data to
 */
static void get_page_links(PageData* page_data) {
  // initialize struct to hold response data
  Response response = { .data = malloc(1), .size = 0 };
  if (response.data == NULL) {
    pthread_mutex_lock(&trace_data.lock);
    trace_data.trace_complete = 1;
    trace_data.status = ERROR_MEM;
    trace_data.err_message = "System memory error.\n"
                             "Mallocing data field in Response struct for page links";
    pthread_mutex_unlock(&trace_data.lock);
    return;
  }

  // initialize struct to hold page data
  page_data->links_titles = malloc(INIT_DATA_ARRAY_SIZE * sizeof(char*));
  page_data->num_links = 0;
  page_data->capacity_links = INIT_DATA_ARRAY_SIZE;
  if (page_data->links_titles == NULL) {
    pthread_mutex_lock(&trace_data.lock);
    trace_data.trace_complete = 1;
    trace_data.status = ERROR_MEM;
    trace_data.err_message = "System memory error.\n"
                             "Mallocing link titles array in PageData struct for page links";
    pthread_mutex_unlock(&trace_data.lock);
    return;
  }

  // build URL for page links
  char url_page_links[URL_LEN] = {0};
  char* url_page_title = curl_easy_escape(curl, page_data->title, 0);
  strcat(url_page_links, url_parts.links_start);
  strcat(url_page_links, url_page_title);
  strcat(url_page_links, url_parts.links_end);

  // request all links on page
  curl_easy_setopt(curl, CURLOPT_URL, url_page_links);
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
  curl_easy_perform(curl);

  // check for parse error
  cJSON* json_data = cJSON_Parse(response.data);
  const char *error_ptr = cJSON_GetErrorPtr();
  if (error_ptr != NULL) {
    fprintf(error_file, "cJSON parse failed. Page links parse error. Error before: %s\n", error_ptr);
    fflush(error_file);
    pthread_mutex_lock(&trace_data.lock);
    trace_data.trace_complete = 1;
    trace_data.status = ERROR_PARSE;
    trace_data.err_message = "Page links parse error. Check error file to see error message.";
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

  // check for continue flag
  cJSON* cont_string_obj;
  cJSON* cont = cJSON_GetObjectItem(json_data, "continue");

  while (cont) {
    // get continue data
    cont = cJSON_GetObjectItem(json_data, "continue");
    cont_string_obj = cJSON_GetObjectItem(cont, "plcontinue");
    char* cont_string = cont_string_obj->valuestring;

    // clear response for next request
    free(response.data);
    response.data = malloc(1);
    response.size = 0;

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
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    curl_easy_perform(curl);

    // check for parse error
    json_data = cJSON_Parse(response.data);
    const char *error_ptr = cJSON_GetErrorPtr();
    if (error_ptr != NULL) {
      fprintf(error_file, "cJSON parse failed. Page links parse error. Error before: %s\n", error_ptr);
      fflush(error_file);
      pthread_mutex_lock(&trace_data.lock);
      trace_data.trace_complete = 1;
      trace_data.status = ERROR_PARSE;
      trace_data.err_message = "Page links parse error. Check error file to see error message.";
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
    if (cont == NULL) { break; }
  }

  // free json data
  cJSON_Delete(json_data);

  // free heap allocated data after last use
  free(response.data);
}


/*
 * Gets the pages entire content in plain text.
 *
 * @param page_title: title of page to get content of
 * @param page: struct to write page content to
 */
char* get_page_content(char* page_title) {
  // initialize struct to hold response data
  Response page_response = { .data = malloc(1), .size = 0 };
  if (page_response.data == NULL) {
    pthread_mutex_lock(&trace_data.lock);
    trace_data.trace_complete = 1;
    trace_data.status = ERROR_MEM;
    trace_data.err_message = "System memory error.\n" 
                             "Mallocing data field in Response struct for page content.";
    pthread_mutex_unlock(&trace_data.lock);
    return NULL;
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
    fprintf(error_file, "cJSON parse failed. Error before: %s\n", error_ptr);
    fflush(error_file);
    pthread_mutex_lock(&trace_data.lock);
    trace_data.trace_complete = 1;
    trace_data.status = ERROR_PARSE;
    trace_data.err_message = "Page content parse error. Check error file to see error message.";
    pthread_mutex_unlock(&trace_data.lock);
    cJSON_Delete(json_data);
    return NULL;
  }

  // extract content
  cJSON* query = cJSON_GetObjectItem(json_data, "query");
  cJSON* pages = cJSON_GetObjectItem(query, "pages");
  cJSON* pages_element = cJSON_GetArrayItem(pages, 0);
  cJSON* pages_extract = cJSON_GetObjectItem(pages_element, "extract");
  char* extract = strdup(pages_extract->valuestring);

  // free json data
  cJSON_Delete(json_data);

  // free heap allocated data
  free(page_response.data);

  return extract;
}


/*
 * Make and continue requests for link data
 *
 * 
 */
void make_links_data_req(PageData* page_data, char* curr_titles, int curr_titles_len) {
  // initialize struct to hold response data
  Response page_response = { .data = malloc(1), .size = 0 };
  if (page_response.data == NULL) {
    pthread_mutex_lock(&trace_data.lock);
    trace_data.trace_complete = 1;
    trace_data.status = ERROR_MEM;
    trace_data.err_message = "System memory error.\n"
                             "Mallocing data field in Response struct for links data.";
    pthread_mutex_unlock(&trace_data.lock);
    return;
  }

  fprintf(file, "%s\n", curr_titles);
  fflush(file);

  // build URL for page content
  char* url_links_data;
  char* url_page_titles = curl_easy_escape(curl, curr_titles,0);
  url_links_data = malloc(strlen(url_parts.intro_start) + strlen(url_page_titles) + strlen(url_parts.intro_end));

  url_links_data[0] = '\0';
  strcat(url_links_data, url_parts.intro_start);
  strcat(url_links_data, url_page_titles);
  strcat(url_links_data, url_parts.intro_end);

  // request page content
  curl_easy_setopt(curl, CURLOPT_URL, url_links_data);
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &page_response);
  curl_easy_perform(curl);

  fprintf(file, "%s\n", page_response.data);
  fflush(file);

  // check for parse error
  cJSON* json_data = cJSON_Parse(page_response.data);
  const char *error_ptr = cJSON_GetErrorPtr();
  if (error_ptr != NULL) {
    fprintf(error_file, "cJSON parse failed. Error before: %s\n", error_ptr);
    fflush(error_file);
    pthread_mutex_lock(&trace_data.lock);
    trace_data.trace_complete = 1;
    trace_data.status = ERROR_PARSE;
    trace_data.err_message = "Links intros parse error. Check error file to see error message.";
    pthread_mutex_unlock(&trace_data.lock);
    cJSON_Delete(json_data);
    return;
  }

  // get intro extract from json object
  cJSON* query = cJSON_GetObjectItem(json_data, "query");
  cJSON* pages = cJSON_GetObjectItem(query, "pages");
  int len = cJSON_GetArraySize(pages);

  for (int i=0; i < len; i++) {
    cJSON* page_element = cJSON_GetArrayItem(pages, i);

    // check if has extract in this round
    cJSON* page_extract = cJSON_GetObjectItem(page_element, "extract");
    if (page_extract != NULL) {
      page_data->links_data[page_data->num_links_data] = cJSON_Duplicate(page_element, 1);
      page_data->num_links_data++;
    }
  }

  // check for continue flag
  cJSON* cont_string_obj;
  cJSON* cont = cJSON_GetObjectItem(json_data, "continue");

  while (cont != NULL) {
    // TODO
  }
 
  free(page_response.data);
}


/*
 * Gets the intros for pages by looping over titles of links. Storing the entire cJSON object
 * from the response. Contains page title + data. Not relying on parallelization with
 * links titles array.
 *
 * @param page_data: struct containing links to get intros for
 */
static void get_links_data(PageData* page_data) {
  // initialize links data array to be same size as num of links
  page_data->num_links_data = 0;
  page_data->links_data = malloc(page_data->num_links * sizeof(cJSON*));
  if (page_data->links_data == NULL) {
    pthread_mutex_lock(&trace_data.lock);
    trace_data.trace_complete = 1;
    trace_data.status = ERROR_MEM;
    trace_data.err_message = "System memory error.\n"
                             "Mallocing array for links data in PageData struct";
    pthread_mutex_unlock(&trace_data.lock);
    return;
  }

  char* curr_titles = NULL;
  int curr_titles_len = 0;
  int link_count = 0;

  for (int i=0; i < page_data->num_links; i++) {
    // space for title + '|' + '\0'
    char* temp = realloc(curr_titles, curr_titles_len + strlen(page_data->links_titles[i]) + 2);
    if (temp == NULL) {
      pthread_mutex_lock(&trace_data.lock);
      trace_data.trace_complete = 1;
      trace_data.status = ERROR_MEM;
      trace_data.err_message = "System memory error.\n"
                               "Reallocing char array for link titles";
      pthread_mutex_unlock(&trace_data.lock);
      return;
    }
    curr_titles = temp;

    // pntr addition to get end of current titles string
    // copy next title there
    memcpy(curr_titles + curr_titles_len, page_data->links_titles[i], strlen(page_data->links_titles[i]));
    curr_titles_len += strlen(page_data->links_titles[i]);

    // add delimiter and null term
    curr_titles[curr_titles_len++] = '|';
    curr_titles[curr_titles_len] = '\0';
    link_count++;


    if (link_count % 50 == 0) {
      // get rid of trailing '|'
      curr_titles[curr_titles_len - 1] = '\0';

      make_links_data_req(page_data, curr_titles, curr_titles_len);

      // reset titles string
      curr_titles[0] = '\0';
      curr_titles_len = 0;
    }
  }

  if (strlen(curr_titles) != 0) {
    make_links_data_req(page_data, curr_titles, curr_titles_len);
  }
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
  if (trace_data.pages_traveled == NULL) {
    trace_data.trace_complete = 1;
    trace_data.status = ERROR_MEM;
    trace_data.err_message = "System memory error.\n"
                             "Mallocing pages traveled array in TraceData struct for run trace.";
    pthread_mutex_unlock(&trace_data.lock);
    return NULL;
  }
  trace_data.pages_traveled[0] = strdup(trace_data.start_page);
  trace_data.num_pages_traveled++;
  pthread_mutex_unlock(&trace_data.lock);

  PageData curr_page;
  int status;
  int trace_complete;

  // set curr page title
  curr_page.title = start_page_title;

  // get start page links
  get_page_links(&curr_page);
  pthread_mutex_lock(&trace_data.lock);
  status = trace_data.status;
  pthread_mutex_unlock(&trace_data.lock);
  if (status != 0) { return NULL; }

  // set destination title in tracer module
  set_dest_page_title(dest_page_title);

  // evaluate curr page for dest page title
  evaluate_page(&curr_page);

  // check if dest page was on start page or error and end if either
  pthread_mutex_lock(&trace_data.lock);
  trace_complete = trace_data.trace_complete;
  status = trace_data.status;
  pthread_mutex_unlock(&trace_data.lock);
  if (trace_complete | status) { return NULL; }

  set_dest_page_content(get_page_content(dest_page_title));

  // leave before looping if encountered errors
  pthread_mutex_lock(&trace_data.lock);
  status = trace_data.status;
  pthread_mutex_unlock(&trace_data.lock);
  if (status != 0) { return NULL; }

  // while destination page not found
  while (trace_complete == 0) {


    //
    // functioning above here
    //


    get_links_data(&curr_page);
    pthread_mutex_lock(&trace_data.lock);
    status = trace_data.status;
    pthread_mutex_unlock(&trace_data.lock);
    if (status != 0) { return NULL; }

    /* for (int i = 0; i < curr_page.num_links; i++) { */
    /*   fprintf(file, "%s\n\n", curr_page.links_data[i]); */
    /* } */
    /* fflush(file); */

    return NULL;

    score_intros(&curr_page);

    char* next_page = get_next_page();

    trace_data.num_pages_traveled++;

    free_page_data(&curr_page);

    curr_page.title = next_page;

    get_page_links(&curr_page);

    evaluate_page(&curr_page);

    pthread_mutex_lock(&trace_data.lock);
    trace_complete = trace_data.trace_complete;
    pthread_mutex_unlock(&trace_data.lock);

  }

  return NULL;
}

