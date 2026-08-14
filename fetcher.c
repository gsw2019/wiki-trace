/**
 * Handles requests for Wikipedia data via curl. Organizes data for the page currently
 * being walked and sends it to tracer module.
 *
 * @author Garret Wilson
 */


#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "fetcher.h"
#include "tracer.h"
#include "cJSON.h"
#include "utils.h"
#include "view.h"


CURL* curl;   // handle used in all fetching/crawling

URLParts url_parts = {
  .verify_start = "https://en.wikipedia.org/w/api.php?action=query&format=json&prop=&titles=",
  .verify_end   = "&redirects=1&formatversion=2",

  .links_start = "https://en.wikipedia.org/w/api.php?action=query&format=json&prop=links&titles=",
  .links_end   = "&formatversion=2&pllimit=max",

  .links_cont_start = "https://en.wikipedia.org/w/api.php?action=query&format=json&prop=links&continue=%7C%7C&titles=",
  .links_cont_mid   = "&formatversion=2&pllimit=max&plcontinue=",

  .intro_start = "https://en.wikipedia.org/w/api.php?action=query&format=json&prop=extracts&titles=",
  .intro_end   = "&redirects=1&formatversion=2&exintro=1&explaintext=1",

  .intro_cont_start = "https://en.wikipedia.org/w/api.php?action=query&format=json&prop=extracts&continue=%7C%7C&titles=",
  .intro_cont_mid   = "&redirects=1&formatversion=2&exintro=1&explaintext=1&excontinue=",

  .content_start = "https://en.wikipedia.org/w/api.php?action=query&format=json&prop=extracts&titles=",
  .content_end   = "&formatversion=2&explaintext=1"
};

FILE* error_file;

FILE* file;

/*
 * Initialize curl and URL pieces required for our requests.
 *
 * @return a handle to curl object
 */
void init_curl()
{
  file = fopen("output.txt", "w");
  if (file == NULL) {
    perror("fopen");
    exit(EXIT_FAILURE);
  }

  curl = curl_easy_init();
  curl_easy_setopt(curl, CURLOPT_USERAGENT, "wiki-trace project (wsg2026@outlook.com)");
}


/*
 * function used by curl to gather and concate the chunks it returns from the response
 *
 * @param ptr:   delivered data
 * @param size:  always 1 (fwrite relic, unused here)
 * @param nmemb: size of delivered data
 * @param res:   the pointer provided in the WRITEDATA setopt
 */
static size_t write_callback(void *ptr, size_t size, size_t nmemb, Response *res)
{
  size_t new_size = res->size + nmemb;    // increase size by adding size of retrieved data
  res->data = realloc(res->data, new_size + 1);
  if (res->data == NULL) { LOG_ERROR(ERROR_REALLOC, NULL, NULL); }
  memcpy(res->data + res->size, ptr, nmemb);    // pointer addition to get destination for new data
  res->size = new_size;
  res->data[res->size] = '\0';    // size as index, puts null term correctly

  return size * nmemb;
}


/*
 * Makes request to check page existence.
 * Sets status in trace data struct if issues.
 *
 * @param page_data: string of json data from response
 * @param page_title: title of wikipedia page
 */
static void check_page_exists(char* page_data, char* page_title)
{
  cJSON* json_data = cJSON_Parse(page_data);
  const char *error_ptr = cJSON_GetErrorPtr();
  if (error_ptr != NULL) { LOG_ERROR(ERROR_CJSON_PARSE, error_ptr, page_data); }

  // walk down JSON object
  cJSON* query = cJSON_GetObjectItem(json_data, "query");
  cJSON* data = cJSON_GetObjectItem(query, "pages");
  cJSON* first_page = cJSON_GetArrayItem(data, 0);
  cJSON* missing = cJSON_GetObjectItem(first_page, "missing");

  if (missing != NULL)
  {
    cJSON_Delete(json_data);
    LOG_ERROR(ERROR_PAGE_EXISTENCE, NULL, page_title);
    return;
  }

  // check if redirects to a different page and change if so
  cJSON* redirect = cJSON_GetObjectItem(query, "redirects");
  if (redirect != NULL)
  {
    cJSON* names = cJSON_GetArrayItem(redirect, 0);
    cJSON* to_name = cJSON_GetObjectItem(names, "to");
    char* redirect_name = to_name->valuestring;

    // update title
    pthread_mutex_lock(&trace_data.lock);
    if (strcmp(page_title, trace_data.start_page) == 0) { strcpy(trace_data.start_page, redirect_name); }
    else if (strcmp(page_title, trace_data.dest_page) == 0) { strcpy(trace_data.dest_page, redirect_name); }
    pthread_mutex_unlock(&trace_data.lock);

    cJSON_Delete(json_data);
    return;
  }

  // check if normalizes to a different page and change if so
  cJSON* normalized = cJSON_GetObjectItem(query, "normalized");
  if (normalized != NULL)
  {
    cJSON* names = cJSON_GetArrayItem(normalized, 0);
    cJSON* to_name = cJSON_GetObjectItem(names, "to");
    char* normalized_name = to_name->valuestring;

    // update title
    pthread_mutex_lock(&trace_data.lock);
    if (strcmp(page_title, trace_data.start_page) == 0) { strcpy(trace_data.start_page, normalized_name); }
    else if (strcmp(page_title, trace_data.dest_page) == 0) { strcpy(trace_data.dest_page, normalized_name); }
    pthread_mutex_unlock(&trace_data.lock);

    cJSON_Delete(json_data);
    return;
  }

  cJSON_Delete(json_data);
}


/*
 * Ensure user entered two pages and the pages exist
 */
void verify_pages()
{
  pthread_mutex_lock(&trace_data.lock);
  char* start_page_title = strdup(trace_data.start_page);
  char* dest_page_title = strdup(trace_data.dest_page);
  pthread_mutex_unlock(&trace_data.lock);

  // turn user input into URL style params
  char* start_page = curl_easy_escape(curl, start_page_title, 0);
  char* dest_page = curl_easy_escape(curl, dest_page_title, 0);

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
  if (start_page_resp.data == NULL) { LOG_ERROR(ERROR_MALLOC, NULL, NULL); }
  curl_easy_setopt(curl, CURLOPT_URL, url_start_page);
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &start_page_resp);
  curl_easy_perform(curl);

  Response dest_page_resp = { .data = malloc(1), .size = 0 };
  if (dest_page_resp.data == NULL) { LOG_ERROR(ERROR_MALLOC, NULL, NULL); }
  curl_easy_setopt(curl, CURLOPT_URL, url_dest_page);
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &dest_page_resp);
  curl_easy_perform(curl);

  // check user input pages status
  check_page_exists(start_page_resp.data, start_page_title); 
  check_page_exists(dest_page_resp.data, dest_page_title);

  free(start_page);
  free(dest_page);
  free(start_page_resp.data);
  free(dest_page_resp.data);
}


/*
 * Gets the link titles out of the JSON data and stores them in PageData struct
 *
 * @param json_data: request response
 * @param page_data: struct to write links to
 */
static void parse_links(cJSON* json_data, PageData* curr_page)
{
  // walk JSON object
  cJSON* query = cJSON_GetObjectItem(json_data,"query");
  cJSON* pages = cJSON_GetObjectItem(query, "pages");
  cJSON* pages_element = cJSON_GetArrayItem(pages, 0);
  cJSON* links_array = cJSON_GetObjectItem(pages_element, "links");

  int capacity = curr_page->links_titles_capacity;    // num of space initialized array has
  int size = curr_page->links_titles_size;    // how many elements we have added
  int len = cJSON_GetArraySize(links_array);

  // dynamically write to page data links titles field
  for (int i=0; i < len; i++)
  {
    // increase size if we dont have room
    if (size >= capacity)
    {
      capacity *= 2;
      char** temp = realloc(curr_page->links_titles, capacity * sizeof(char*));
      if (temp == NULL) { LOG_ERROR(ERROR_REALLOC, NULL, NULL); }

      curr_page->links_titles_capacity = capacity;
      curr_page->links_titles = temp;
    }

    // get array element with link info
    cJSON* links_element = cJSON_GetArrayItem(links_array, i);

    //
    // can add conditional for only ns:0, only main articles
    //

    // allocate mem for individual title
    cJSON* link = cJSON_GetObjectItem(links_element, "title");
    curr_page->links_titles[size] = malloc(strlen(link->valuestring) + 1);
    if (curr_page->links_titles[size] == NULL) { LOG_ERROR(ERROR_MALLOC, NULL, NULL); }

    // copy string into allocated mem
    strcpy(curr_page->links_titles[size], link->valuestring);

    size++;
  }

  curr_page->links_titles_size = size;
}


/*
 * Gets all of the specified page's links
 *
 * @param page_title: title of page to get links of
 * @param curr_page: struct to write page data to
 */
void get_page_links(PageData* curr_page)
{
  // initialize struct to hold response data
  Response response = { .data = malloc(1), .size = 0 };
  if (response.data == NULL) { LOG_ERROR(ERROR_MALLOC, NULL, NULL); }

  // initialize struct to hold page data
  curr_page->links_titles = malloc(INIT_DATA_ARRAY_SIZE * sizeof(char*));
  curr_page->links_titles_size = 0;
  curr_page->links_titles_capacity = INIT_DATA_ARRAY_SIZE;
  if (curr_page->links_titles == NULL) { LOG_ERROR(ERROR_MALLOC, NULL, NULL); }

  char* url_page_title = curl_easy_escape(curl, curr_page->title, 0);
  int cont = 1;
  int in_cont = 0;
  char* cont_string;
  while (cont)
  {
    char url[URL_LEN] = {0};
    if (in_cont)
    {
      // build continue URL
      char* url_cont_string = curl_easy_escape(curl, cont_string, 0);
      strcat(url, url_parts.links_cont_start);
      strcat(url, url_page_title);
      strcat(url, url_parts.links_cont_mid);
      strcat(url, url_cont_string);
    }
    else
    {
      // build initial URL
      strcat(url, url_parts.links_start);
      strcat(url, url_page_title);
      strcat(url, url_parts.links_end);
    }

    // make request
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    curl_easy_perform(curl);

    /* fprintf(file, "response data: %s\n\n", response.data); */
    /* fflush(file); */

    // check for parse error
    cJSON* json_data = cJSON_Parse(response.data);
    const char *error_ptr = cJSON_GetErrorPtr();
    if (error_ptr != NULL) { LOG_ERROR(ERROR_CJSON_PARSE, error_ptr, NULL); }

    // extract links
    parse_links(json_data, curr_page);

    // cleanup response
    free(response.data);
    response.data = malloc(1);
    response.size = 0;

    // check if has continue flag
    cJSON* cont_obj = cJSON_GetObjectItem(json_data, "continue");
    if  (cont_obj != NULL) 
    {
      cJSON* cont_string_obj = cJSON_GetObjectItem(cont_obj, "plcontinue");
      cont_string = strdup(cont_string_obj->valuestring);
      cJSON_Delete(json_data);
      in_cont = 1; 
    }
    else
    {
      cont = 0; 
      cJSON_Delete(json_data);
    }
  }

  // free heap allocated data after last use
  free(url_page_title);
  free(response.data);
}


/*
 * Gets the pages entire content in plain text.
 *
 * @param page_title: title of page to get content of
 * @param page: struct to write page content to
 */
char* get_page_content(char* page_title)
{
  // initialize struct to hold response data
  Response page_response = { .data = malloc(1), .size = 0 };
  if (page_response.data == NULL) { LOG_ERROR(ERROR_MALLOC, NULL, NULL); }

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
  if (error_ptr != NULL) { LOG_ERROR(ERROR_CJSON_PARSE, error_ptr, page_response.data); }

  // extract content
  cJSON* query = cJSON_GetObjectItem(json_data, "query");
  cJSON* pages = cJSON_GetObjectItem(query, "pages");
  cJSON* pages_element = cJSON_GetArrayItem(pages, 0);
  cJSON* pages_extract = cJSON_GetObjectItem(pages_element, "extract");
  char* extract = strdup(pages_extract->valuestring);

  /* fprintf(file, "%s\n\n", extract); */
  /* fflush(file); */

  // free json data
  cJSON_Delete(json_data);

  // free heap allocated data
  free(page_response.data);

  return extract;
}


/*
 * write the cJSON objects to PageData struct
 *
 * @param json_data: the parsed response data
 * @param page_data: struct to write the cJSON object to
 */
static void parse_links_data(cJSON* json_data, PageData* curr_page)
{
  // walk JSON obejct
  cJSON* query = cJSON_GetObjectItem(json_data, "query");
  cJSON* pages = cJSON_GetObjectItem(query, "pages");
  int len = cJSON_GetArraySize(pages);

  for (int i=0; i < len; i++)
  {
    cJSON* page_element = cJSON_GetArrayItem(pages, i);

    // check if has extract in this round
    cJSON* page_extract = cJSON_GetObjectItem(page_element, "extract");
    if (page_extract != NULL)
    {
      curr_page->links_data[curr_page->links_data_size] = cJSON_Duplicate(page_element, 1);
      curr_page->links_data_size++;

      cJSON* page_title = cJSON_GetObjectItem(page_element, "title");
    }
  }
}


/*
 * Make and continue requests for link data
 *
 * @param page_data: where to store cJSON object with extract
 * @param curr_titles: string of titles
 */
static void make_links_data_req(char* curr_titles, PageData* curr_page)
{
  // initialize struct to hold response data
  Response response = { .data = malloc(1), .size = 0 };
  if (response.data == NULL) { LOG_ERROR(ERROR_MALLOC, NULL, NULL); }

  // check for continue flag
  char* url_page_titles = curl_easy_escape(curl, curr_titles,0);
  cJSON* json_data;
  int cont = 1;
  int in_cont = 0;
  char cont_str[32];

  while (cont)
  {
    char* url;
    if (in_cont)
    {
      // build continue URL
      url = malloc(strlen(url_parts.intro_cont_start) + strlen(url_page_titles) +
                              strlen(url_parts.intro_cont_mid) + strlen(cont_str) + 1);
      if (url == NULL) { LOG_ERROR(ERROR_MALLOC, NULL, NULL); }

      url[0] = '\0';
      strcat(url, url_parts.intro_cont_start);
      strcat(url, url_page_titles);
      strcat(url, url_parts.intro_cont_mid);
      strcat(url, cont_str);
    }
    else
   {
      // build initial URL
      url = malloc(strlen(url_parts.intro_start) + strlen(url_page_titles) + strlen(url_parts.intro_end) + 1);
      if (url == NULL) { LOG_ERROR(ERROR_MALLOC, NULL, NULL); }

      url[0] = '\0';
      strcat(url, url_parts.intro_start);
      strcat(url, url_page_titles);
      strcat(url, url_parts.intro_end);
    }


    // request page content
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    curl_easy_perform(curl);
 
    // check for parse error
    json_data = cJSON_Parse(response.data);
    const char *error_ptr = cJSON_GetErrorPtr();
    if (error_ptr != NULL) { LOG_ERROR(ERROR_CJSON_PARSE, error_ptr, response.data); }

    parse_links_data(json_data, curr_page);

    free(response.data);
    response.data = malloc(1);
    response.size = 0;

    // check if contiunue flag
    cJSON* cont_obj = cJSON_GetObjectItem(json_data, "continue");
    if (cont_obj != NULL)
    {
      cJSON* cont_int_obj = cJSON_GetObjectItem(cont_obj, "excontinue");
      int cont_int = cont_int_obj->valueint;

      // convert to string
      snprintf(cont_str, sizeof(cont_str), "%d", cont_int);
      in_cont = 1;
    }
    else { cont = 0; }

    free(url);
  }

  // free json data
  cJSON_Delete(json_data);

  // free heap allocated data
  free(url_page_titles);
  free(response.data);
}


/*
 * Build string of titles for request URL out of links gathered from a page. Each string can contain
 * 50 titles max. After looping over 50 titles, we make request with helper functions
 *
 * @param page_data: struct containing links to get intros for
 */
void get_links_data(PageData* curr_page)
{
  // initialize links data array to be same size as num of link titles
  curr_page->links_data_size = 0;
  curr_page->links_data_capacity = curr_page->links_titles_size;
  curr_page->links_data = malloc(curr_page->links_titles_size * sizeof(cJSON*));
  if (curr_page->links_data == NULL) { LOG_ERROR(ERROR_MALLOC, NULL, NULL); }

  char* curr_titles = NULL;
  int curr_titles_len = 0;
  int link_count = 0;

  for (int i=0; i < curr_page->links_titles_size; i++)
  {
    // space for title + '|' + '\0'
    char* temp = realloc(curr_titles, curr_titles_len + strlen(curr_page->links_titles[i]) + 2);
    if (temp == NULL) { LOG_ERROR(ERROR_REALLOC, NULL, NULL); }
    curr_titles = temp;

    // pntr addition to get end of current titles string
    // copy next title there
    memcpy(curr_titles + curr_titles_len, curr_page->links_titles[i], strlen(curr_page->links_titles[i]));
    curr_titles_len += strlen(curr_page->links_titles[i]);

    // add delimiter and null term
    curr_titles[curr_titles_len++] = '|';
    curr_titles[curr_titles_len] = '\0';
    link_count++;

    if (link_count % 50 == 0)
    {
      // get rid of trailing '|'
      curr_titles[curr_titles_len - 1] = '\0';

      make_links_data_req(curr_titles, curr_page);

      // reset titles string
      curr_titles[0] = '\0';
      curr_titles_len = 0;
    }
  }

  // num of titles not multiple of 50
  if (strlen(curr_titles) != 0)
  {
    make_links_data_req(curr_titles, curr_page);
  }
}

