/**
 * function implementations for the fetcher logic
 *
 * @author Garret Wilson
 */


#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "fetcher.h"
#include "cJSON.h"


CURL* curl;   // handle used in all fetching/crawling

URLParts url_parts;   // the url pieces needed for requests


/*
 * initialize curl for the program
 *
 * @return a handle to curl object
 */
CURL* init_curl() {
  curl = curl_easy_init();
  curl_easy_setopt(curl, CURLOPT_USERAGENT, "wiki-trace project (wsg2026@outlook.com)");

  url_parts.verify_start = "https://en.wikipedia.org/w/api.php?action=query&format=json&prop=&titles=";
  url_parts.verify_end = "&formatversion=2";

  url_parts.intro_start = "https://en.wikipedia.org/w/api.php?action=query&format=json&prop=extracts&titles=";
  url_parts.intro_end = "&formatversion=2&exintro=1&explaintext=1";

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
 * Verify user input. Returns different error values depending on the error.
 *
 * @param data: a struct containing vars to be unpacked by the worker thread
 *
 * @return 1: missing one page or both from input data
 * @return 2: cJSON parse failed
 * @return 3: a page does not exist
 */
void* verify_pages(void* args) {
  TraceData* trace_data = (TraceData *) args;
  // if missing one or both titles, return err value
  pthread_mutex_lock(&trace_data->lock);
  if (strlen(trace_data->start_page) == 0 || strlen(trace_data->dest_page) == 0) { 
    trace_data->status = 1;
    pthread_mutex_unlock(&trace_data->lock);
    return NULL;
  }
  pthread_mutex_unlock(&trace_data->lock);

  // turn user input into URL style params
  pthread_mutex_lock(&trace_data->lock);
  char* start_page = curl_easy_escape(curl, trace_data->start_page, 0);
  char* dest_page = curl_easy_escape(curl, trace_data->dest_page, 0);
  pthread_mutex_unlock(&trace_data->lock);

  // build URLs
  char url_start_page[350] = {0};
  strcat(url_start_page, url_parts.verify_start);
  strcat(url_start_page, start_page);
  strcat(url_start_page, url_parts.verify_end);

  char url_dest_page[350] = {0};
  strcat(url_dest_page, url_parts.verify_start);
  strcat(url_dest_page, dest_page);
  strcat(url_dest_page, url_parts.verify_end);

  // make requests
  Response start_page_resp = { .data = malloc(1), .size = 0 };
  curl_easy_setopt(curl, CURLOPT_URL, url_start_page);
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &start_page_resp);
  curl_easy_perform(curl);

  Response dest_page_resp = { .data = malloc(1), .size = 0 };
  curl_easy_setopt(curl, CURLOPT_URL, url_dest_page);
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &dest_page_resp);
  curl_easy_perform(curl);

  /* printf("start url: %s\n", url_start_page); */
  /* printf("\n"); */
  /* printf("dest url: %s\n", url_dest_page); */
  /**/
  /* printf("%s\n", start_page_resp.data); */
  /* printf("\n\n"); */
  /* printf("%s\n", dest_page_resp.data); */
  /**/

  int start_page_status = check_page_exists(start_page_resp.data);

  if (start_page_status == 2) {
    pthread_mutex_lock(&trace_data->lock);
    trace_data->status = 2;
    trace_data->err_message = "Start page parse error. Quit to see error message.";
    pthread_mutex_unlock(&trace_data->lock);

    free(start_page_resp.data);
    free(dest_page_resp.data);

    return NULL;
  }
  else if (start_page_status == 3) {
    pthread_mutex_lock(&trace_data->lock);
    trace_data->status = 3;
    trace_data->err_message = "Start page does not exists.";
    pthread_mutex_unlock(&trace_data->lock);

    free(start_page_resp.data);
    free(dest_page_resp.data);

    return NULL;
  }

  int dest_page_status = check_page_exists(dest_page_resp.data);

  if (dest_page_status == 2) {
    pthread_mutex_lock(&trace_data->lock);
    trace_data->status = 2;
    trace_data->err_message = "Dest page parse error. Quit to see error message.";
    pthread_mutex_unlock(&trace_data->lock);

    free(start_page_resp.data);
    free(dest_page_resp.data);

    return NULL;
  }
  else if (dest_page_status == 3) {
    pthread_mutex_lock(&trace_data->lock);
    trace_data->status = 3;
    trace_data->err_message = "Dest page does not exists.";
    pthread_mutex_unlock(&trace_data->lock);

    free(start_page_resp.data);
    free(dest_page_resp.data);

    return NULL;
  }

  free(start_page_resp.data);
  free(dest_page_resp.data);

  pthread_mutex_lock(&trace_data->lock);
  trace_data->status = 0;
  pthread_mutex_unlock(&trace_data->lock);

  return NULL;
}


/*
 * checks response data to see if the Wikipedia page exists
 *
 * @param page_data: string of json data from response
 * @return 0: page exists
 * @return 2: jsom parse error
 * @return 3: page is missing
 */
static int check_page_exists(char* page_data) {
  cJSON* json_data = cJSON_Parse(page_data);
  const char *error_ptr = cJSON_GetErrorPtr();
  if (error_ptr != NULL) { 
    fprintf(stderr, "cJSON parse failed. Error before: %s\n", error_ptr);
    cJSON_Delete(json_data);
    return 2;
  }

  // walk down JSON object
  cJSON* query = cJSON_GetObjectItem(json_data, "query");
  cJSON* data = cJSON_GetObjectItem(query, "pages");
  cJSON* first_page = cJSON_GetArrayItem(data, 0);
  cJSON* missing = cJSON_GetObjectItem(first_page, "missing");

  if (missing != NULL) {
    cJSON_Delete(data);
    return 3;
  }

  cJSON_Delete(data);

  return 0;
}

