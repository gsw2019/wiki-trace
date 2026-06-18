/**
 * Prototypes, structs, and macros for for fetching Wikipedia data
 *
 * @author Garret Wilson
 */


#include <curl/curl.h>

#include "view.h"
#include "cJSON.h"


typedef struct {
  char* verify_start;
  char* verify_end;
  char* intro_start;
  char* intro_end;
} URLParts;


typedef struct {
  char *data;
  size_t size;
} Response;

typedef struct {
  int status;
  char* message;
} VerificationData;


// init or utility funcs
CURL* init_curl();
static size_t write_callback(void *ptr, size_t size, size_t nmemb, Response *res);

// verifying pages input by user
void* verify_pages(void* args);
static int check_page_exists(char* page_data);

