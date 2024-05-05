#include "digest.h"

#include <assert.h>
#include <errno.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "lllist.h"
#include "macros.h"
#include "sched.h"
#include "worker.h"

static int digest_ret_val = EXIT_FAILURE;

void *digest_worker(void *arg) {
  worker_args_t *worker_args;
  lllist_t list;
  char *data;
  size_t data_len;
  bool has_data;

  worker_args = (worker_args_t *)arg;
  list = worker_args->list;

  (void)list;
  (void)data;
  (void)data_len;
  (void)has_data;

  data = NULL;
  data_len = 0;
  has_data = true;

  while (has_data) {
    switch (lllist_node_consume(&list, &data, &data_len)) {
    case LLLIST_SUCCESS:
#if LOG_LVL >= INFO
      fprintf(stderr, "INFO(digest_worker): list->first: %p\n",
              (void *)list.first);
#endif
      fprintf(stdout, "DIGEST: %s\n", data);
      break;
    case LLLIST_NO_DATA:
#if LOG_LVL >= DEBUG
      fprintf(stderr, "DEBUG(digest_worker): list->first: %p NO DATA\n",
              (void *)list.first);
#endif
      if (sched_yield() != 0) {
        perror("digest worker sched yield");
      }
      break;
    case LLLIST_ENDED:
#if LOG_LVL >= INFO
      fprintf(stderr, "INFO(digest_worker): list->first: %p END\n",
              (void *)list.first);
#endif
      has_data = false;
      break;
    case LLLIST_FAILURE:
      goto digest_cleanup;
    default:
      assert(!"Should never get here");
      break;
    }
  }

  digest_ret_val = EXIT_SUCCESS;
  goto digest_cleanup;

digest_cleanup:
  if (data) {
    free(data);
  }

#if LOG_LVL >= INFO
  fprintf(stderr, "INFO(digest_worker): exit with code %d\n", digest_ret_val);
#endif

  pthread_exit(&digest_ret_val);
}
