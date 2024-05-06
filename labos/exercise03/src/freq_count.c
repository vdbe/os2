#include "freq_count.h"

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

static int freq_count_ret_val = EXIT_FAILURE;

static inline size_t process_data(
	char* data,
	size_t data_len,
	size_t freq_count[]
) {
	size_t count = 0;

	for (size_t ii = 0; ii < data_len -1; ii++) {
		char c =  data[ii] | 0x20; //  0b00100000;

		if (c < 'a' || c > 'z') {
			continue;
		}

		freq_count[c - 'a'] += 1;
		count += 1;
	}

	return count;
}

void *freq_count_worker(void *arg) {
  worker_args_t *worker_args;
  lllist_t list;
  char *data;
  size_t data_len;
	size_t data_buf_len;
  bool has_data;

	size_t total_count = 0;
	size_t freq_count['z' - 'a' + 1];

  worker_args = (worker_args_t *)arg;
  list = worker_args->list;

  (void)list;
  (void)data;
  (void)data_len;
  (void)has_data;

  data = NULL;
  data_buf_len = 0;
  has_data = true;

  while (has_data) {
    switch (lllist_node_consume(&list, &data, &data_len, &data_buf_len)) {
    case LLLIST_SUCCESS:
#if LOG_LVL >= INFO
      fprintf(stderr, "INFO(freq_count_worker): list->first: %p\n",
              (void *)list.first);
#endif

      total_count += process_data(data, data_len, freq_count);
      break;
    case LLLIST_NO_DATA:
#if LOG_LVL >= DEBUG
      fprintf(stderr, "DEBUG(freq_count_worker): list->first: %p NO DATA\n",
              (void *)list.first);
#endif
      if (sched_yield() != 0) {
        perror("freq_count worker sched yield");
      }
      break;
    case LLLIST_ENDED:
#if LOG_LVL >= INFO
      fprintf(stderr, "INFO(freq_count_worker): list->first: %p END\n",
              (void *)list.first);
#endif
      has_data = false;
      break;
    case LLLIST_FAILURE:
      goto freq_count_cleanup;
    default:
      assert(!"Should never get here");
      break;
    }
  }

  freq_count_ret_val = EXIT_SUCCESS;
  goto freq_count_cleanup;

freq_count_cleanup:
  if (data) {
    free(data);
  }

	for (size_t ii = 0; ii < 'z' - 'a' + 1; ii++) {
		fprintf(stdout, "%c: %ld\n", (char)('a' + ii), freq_count[ii]);
	}

#if LOG_LVL >= INFO
  fprintf(stderr, "INFO(freq_count_worker): exit with code %d\n",
          freq_count_ret_val);
#endif

  pthread_exit(&freq_count_ret_val);
}
