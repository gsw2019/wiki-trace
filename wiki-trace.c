/*
 * Entry point for the wiki-trace game solver
 *
 * @author Garret Wilson
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#include "view.h"
#include "fetcher.h"
#include "utils.h"


int main(int argc, char* argv[]) {
  // initialize utilities
  init_utils();

  // initialize curl
  CURL* curl = init_curl();

  // launch TUI
  init_view();

  // cleanup
  curl_easy_cleanup(curl);

  return 0;
}
