/*
 * entry point for the wiki-trace game solver
 *
 * @author Garret Wilson
 */


#include <curl/curl.h>
#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "view.h"


typedef struct {
    char *data;
    size_t size;
} Response;

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

int main(int argc, char* argv[]) {
  init_wiki_trace_view();
  TracePages pages = get_trace_pages(); 

  clear();
  refresh();
  endwin();

  printf("starting page: %s\n", pages.start_page);
  printf("destination page: %s\n", pages.dest_page);

  /* Response res = { .data = malloc(1), .size = 0 }; */
  /**/
  /* CURL *curl = curl_easy_init(); */
  /* curl_easy_setopt(curl, CURLOPT_USERAGENT, "wiki-trace project (wsg2026@outlook.com)"); */
  /* curl_easy_setopt(curl, CURLOPT_URL, "https://en.wikipedia.org/w/api.php?action=query&titles=Earth&prop=links&pllimit=max&format=json"); */
  /* curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback); */
  /* curl_easy_setopt(curl, CURLOPT_WRITEDATA, &res); */
  /* curl_easy_perform(curl); */
  /**/
  /* printf("%s/n", res.data); */
  /**/
  /* curl_easy_cleanup(curl); */
  /* free(res.data); */
  /**/

  return 0;
}
