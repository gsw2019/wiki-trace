/**
 * Generic utilities used across the wiki-trace project
 *
 * @author Garret Wilson
 */


#include <stdio.h>
#include <string.h>

#include "utils.h"
#include "fetcher.h"
#include "view.h"


FILE* err_file;

char delim[50];

/*
 * Initialize some vars for other utililities functions
 */
void init_utils() {
  // error log file
  err_file = fopen("error_output_wt.txt", "w");

  // delimiter to seperate error logs 
  memset(delim, '=', 48);
  delim[49] = '\0';
}


/*
 * Generic function called to log errors to text file and set vars in the TraceData global struct
 *
 * @param func: the name of the function that called this function
 * @param line: the line number where this function was called
 * @param tag: the enum describing the type of error
 * @param error_ptr: the pointer to where cJSON parse failed
 */
void log_error(const char* func, int line, ErrTag tag, const char* error_ptr, void* specifier) {
  err_file = fopen("error_output_wt.txt", "w");

  switch (tag) {
    case ERROR_CJSON_PARSE:
      pthread_mutex_lock(&trace_data.lock);
      trace_data.trace_complete = 1;
      trace_data.status = ERROR_CJSON_PARSE;
      trace_data.err_message = "cJSON parse error. See error_output_wt.txt file.";
      pthread_mutex_unlock(&trace_data.lock);

      // prints further info to error file
      fprintf(err_file, "\n%s\n", delim);
      fprintf(err_file, "%s\n", "CJSON PARSE ERROR");
      fprintf(err_file, "Error in: %s, line %d\n", func, line);
      fprintf(err_file, "Error pointer: %s", error_ptr);
      if (specifier != NULL) { fprintf(err_file, "Error specifier: %s", (char*) specifier); }
      fprintf(err_file, "%s", delim);
      fflush(err_file);
      fclose(err_file);
      break;

    case ERROR_USER_INPUT:
      pthread_mutex_lock(&trace_data.lock);
      trace_data.trace_complete = 1;
      trace_data.status = ERROR_USER_INPUT;
      trace_data.err_message = "Missing page titles. Please enter two Wikipedia pages.";
      pthread_mutex_unlock(&trace_data.lock);
      break;

    // used for start and dest pages
    case ERROR_PAGE_EXISTENCE:
      pthread_mutex_lock(&trace_data.lock);
      trace_data.trace_complete = 1;
      trace_data.status = ERROR_PAGE_EXISTENCE;
      if ( (char*)specifier == trace_data.start_page ) {
        trace_data.err_message = "Start page does not exists. Make sure to enter the page title\n"
                                 "exactly as it appears on Wikipedia. Copy and paste is acceptable.";
      }
      else if ( (char*)specifier == trace_data.dest_page ) {
        trace_data.err_message = "Dest page does not exists. Make sure to enter the page title\n"
                                 "exactly as it appears on Wikipedia. Copy and paste is acceptable.";
      }
      else {
        trace_data.err_message = "Unknown page does not exist";
      }
      pthread_mutex_unlock(&trace_data.lock);
      break;

    case ERROR_FILE:
      pthread_mutex_lock(&trace_data.lock);
      trace_data.trace_complete = 1;
      trace_data.status = ERROR_FILE;
      trace_data.err_message = "Opening file error. See error_output_wt.txt file.";
      pthread_mutex_unlock(&trace_data.lock);

      // prints further info to errorr file
      fprintf(err_file, "\n%s\n", delim);
      fprintf(err_file, "%s\n", "FILE ERROR"); 
      fprintf(err_file, "Error in: %s, line %d\n", func, line);
      if (specifier != NULL) { fprintf(err_file, "%s\n", (char*) specifier); }
      fprintf(err_file, "%s", delim);
      fflush(err_file);
      fclose(err_file);
      break;

    case ERROR_MALLOC:
    case ERROR_REALLOC:
    case ERROR_CALLOC:
      pthread_mutex_lock(&trace_data.lock);
      trace_data.init_complete = 1;
      if (tag == ERROR_MALLOC) { trace_data.status = ERROR_MALLOC; }
      else if (tag == ERROR_REALLOC) { trace_data.status = ERROR_REALLOC; }
      else if (tag == ERROR_CALLOC) { trace_data.status = ERROR_CALLOC; }
      trace_data.status = ERROR_MALLOC;
      trace_data.err_message = "System memory error.";
      pthread_mutex_unlock(&trace_data.lock);

      // print further info to error file
      fprintf(err_file, "\n%s\n", delim);
      if (tag == ERROR_MALLOC) { fprintf(err_file, "%s\n", "MALLOC ERROR"); }
      else if (tag == ERROR_REALLOC) { fprintf(err_file, "%s\n", "REALLOC ERROR"); }
      else if (tag == ERROR_CALLOC) { fprintf(err_file, "%s\n", "CALLOC ERROR"); }
      fprintf(err_file, "Error in: %s, line %d\n", func, line);
      fprintf(err_file, "%s", delim);
      fflush(err_file);
      fclose(err_file);
      break;

    default:
      fprintf(err_file, "\n%s\n", delim);
      fprintf(err_file, "%s", "UNKNOWN ERROR");
      fprintf(err_file, "%s", delim);
      fflush(err_file);
      fclose(err_file);
      break;
  }
}

