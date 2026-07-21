/**
 * Prototypes, macros, and structs for generic utilities for wiki-trace
 *
 * @author Garret Wilson
 */


#ifndef UTILS_H
#define UTILS_H

#include <stdio.h>

#define LOG_ERROR(tag, err_ptr, specifier) log_error(__func__, __LINE__, tag, err_ptr, specifier)


typedef enum {
  ERROR_MALLOC = 1,
  ERROR_CALLOC = 2,
  ERROR_REALLOC = 3,
  ERROR_CJSON_PARSE = 4,
  ERROR_USER_INPUT = 5,
  ERROR_PAGE_EXISTENCE = 6,
  ERROR_FILE = 7
} ErrTag;


void init_utils();
void log_error(const char* func, int line, ErrTag tag, const char* err_ptr, void* specifier);


#endif

