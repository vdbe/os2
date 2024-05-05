#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "macros.h"

#include "lllist.h"
#include "worker.h"

#include "digest.h"
#include "freq_count.h"
#include "reader.h"

#define INPUT_FILE "inputs.txt"

int main(int argc, char *argv[]) {
  (void)argc;
  (void)argv;

  int ret;
  int *pthread_ret_val;
  char *input_file;

  pthread_attr_t attr;
  worker_args_t worker_args;

  ret = EXIT_FAILURE;
  input_file = INPUT_FILE;

  worker_args = (worker_args_t){.extra = input_file};

  union {
    pthread_t index[3];
    struct {
      pthread_t reader;
      pthread_t digest;
      pthread_t count;
    } name;

  } threads = {.name = {0, 0, 0}};

#if LOG_LVL >= DEBUG
#ifdef TSAN_ENABLED
  fprintf(stderr, "DEBUG(main): TSAN enabled\n");
#endif
#endif

  if (lllist_init(&(worker_args.list), 2) != LLLIST_SUCCESS) {
    perror("lllist_init");
    goto main_return;
  }

  worker_args.extra = input_file;

  if (setup_pthread_attr(&attr) != 0) {
    goto main_cleanup;
  }

  if (pthread_create(&threads.name.reader, &attr, &reader_worker,
                     &worker_args) != 0) {
    perror("pthread create");
    pthread_attr_destroy(&attr);
    goto main_cleanup;
  }

  if (pthread_create(&threads.name.digest, &attr, &digest_worker,
                     &worker_args) != 0) {
    perror("pthread create");
    pthread_attr_destroy(&attr);
    goto main_cleanup;
  }

  if (pthread_create(&threads.name.count, &attr, &freq_count_worker,
                     &worker_args) != 0) {
    perror("pthread create");
    pthread_attr_destroy(&attr);
    goto main_cleanup;
  }

  // Cleanup `attr`
  if (pthread_attr_destroy(&attr) != 0) {
    perror("pthread attr destroy");
    goto main_cleanup;
  }

  for (size_t thread_index = 0;
       (thread_index < (sizeof(threads) / sizeof(threads.index[0])));
       thread_index++) {
    if (threads.index[thread_index] == 0) {
      goto main_cleanup;
    }

    // Wait for thread to exit
    if (pthread_join(threads.index[thread_index], (void **)&pthread_ret_val) !=
        0) {
      perror("pthread join");
      break;
    }
    threads.index[thread_index] = 0;

    if (pthread_ret_val == PTHREAD_CANCELED) {
      fprintf(stderr, "thread canceled\n");
      break;
    } else if (*pthread_ret_val != EXIT_SUCCESS) {
      fprintf(stderr, "thread exited with code %d\n", *pthread_ret_val);
    }
  };

  ret = EXIT_SUCCESS;

main_cleanup:
  for (size_t thread_index = 0;
       thread_index < (sizeof(threads) / sizeof(threads.index[0]));
       thread_index++) {
    if (threads.index[thread_index] == 0) {
      // Thread already stopped/joined
      continue;
    }

    if (pthread_cancel(threads.index[thread_index])) {
      perror("main pthread cancel");
    }

    // Wait for thread to cancel
    if (pthread_join(threads.index[thread_index], (void **)&pthread_ret_val) !=
        0) {
      perror("pthread join");
      break;
    }
    threads.index[thread_index] = 0;

    if (pthread_ret_val == PTHREAD_CANCELED) {
      fprintf(stderr, "thread canceled\n");
      continue;
    } else if (*pthread_ret_val != EXIT_SUCCESS) {
      fprintf(stderr, "thread exited with code %d\n", *pthread_ret_val);
    }
  }

  lllist_free(&worker_args.list);

main_return:
#if LOG_LVL >= INFO
  fprintf(stderr, "INFO(main): exit with code %d\n", ret);
#endif

  return ret;
}
