/*
 * Prototypes, macros, and structs for buildding a heap/priority queue
 *
 * @author Garret Wilson
 */


#ifndef HEAP_H
#define HEAP_H


#include "tracer.h"
#include "utils.h"


typedef struct {
  Paths paths[];
  int size;
  int capacity;
} Heap;




#endif  // HEAP_H

