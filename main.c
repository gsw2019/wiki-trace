/*
 * Entry point for the wiki-trace game solver
 *
 * @author Garret Wilson
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "view.h"
#include "fetcher.h"

FILE* file;

int main(int argc, char* argv[]) {

  file = fopen("output.txt", "w");

  // initialize curl
  CURL* curl = init_curl();

  // launch TUI
  init_view();

  // finished with curl
  curl_easy_cleanup(curl);

  fprintf(file, "main returning\n");
  fflush(file);
  return 0;
}
