#pragma once

#include "lllist.h"

typedef struct worker_args {
  lllist_t list;

  // Woker specific data
  void *extra;
} worker_args_t;

int setup_pthread_attr(pthread_attr_t *);
