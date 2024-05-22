#include "digest.h"

#include <assert.h>
#include <errno.h>
#include <openssl/sha.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "lllist.h"
#include "macros.h"
#include "sched.h"
#include "worker.h"

static int digest_ret_val = EXIT_FAILURE;

static inline int process_data(
	char* data,
	size_t data_len,
	unsigned char* sha1_binary_buffer,
	char* sha1_hex_buffer,
	FILE* fptr
) {
	// NOTE: `data_len - 1` to ignore the `\0` denoting the end of `data`
	SHA1((unsigned char*)data, data_len-1, sha1_binary_buffer);

	for(size_t ii = 0; ii < SHA_DIGEST_LENGTH; ii++) {
		sprintf(&sha1_hex_buffer[ii*2], "%02x", sha1_binary_buffer[ii]);
	}

#if LOG_LVL >= INFO
	fprintf(stderr, "INFO(digest_worker): %s - [%s]\n", sha1_hex_buffer, data);
#endif
	fprintf(fptr, "%s\n", sha1_hex_buffer);

	return 0;
}

void *digest_worker(void *arg) {
  worker_args_t *worker_args;
  lllist_t list;
  char *data;
	size_t data_len;
  size_t data_buf_len;
  bool has_data;
	unsigned char sha1_binary_buffer[SHA_DIGEST_LENGTH];
	char sha1_hex_buffer[SHA_DIGEST_LENGTH*2 + 1];
	FILE *fptr;
	const char *digest_file;

	SLEEP(100);

	sha1_hex_buffer[SHA_DIGEST_LENGTH*2] = '\0';

  worker_args = (worker_args_t *)arg;
	digest_file = worker_args->extra;
  list = worker_args->list;

	fptr = fopen(digest_file, "w+");
	if (fptr == NULL) {
		perror("Open DIGEST_FILE");
		goto digest_return;
	}

  data = NULL;
  data_buf_len = 0;
  has_data = true;

  while (has_data) {
    switch (lllist_node_consume(&list, &data, &data_len, &data_buf_len)) {
    case LLLIST_SUCCESS:
#if LOG_LVL >= INFO
      fprintf(stderr, "INFO(digest_worker): list->first: %p\n",
              (void *)list.first);
#endif
      process_data(data, data_len, sha1_binary_buffer, sha1_hex_buffer, fptr);
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
	fclose(fptr);

  if (data) {
    free(data);
  }

digest_return:
#if LOG_LVL >= INFO
  fprintf(stderr, "INFO(digest_worker): exit with code %d\n", digest_ret_val);
#endif

  pthread_exit(&digest_ret_val);
}
