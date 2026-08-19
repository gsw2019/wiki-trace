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
#include "logger.h"


int main(int argc, char* argv[]) {
  // initialize utilities
  init_logger();

  // initialize curl
  init_curl();

  // launch TUI
  init_view();

  return 0;
}
