#include "worker.h"

#include <pthread.h>
#include <limits.h>
#include <stdio.h>

inline int setup_pthread_attr(pthread_attr_t *attr) {
  if (pthread_attr_init(attr) != 0) {
    perror("pthread attr init");
    return 1;
  }

  if (pthread_attr_setstacksize(attr, PTHREAD_STACK_MIN) != 0) {
    perror("pthread set stacksize");
    pthread_attr_destroy(attr);
    return 2;
  }

  return 0;
}
