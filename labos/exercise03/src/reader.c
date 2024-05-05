#include "reader.h"

#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#include "lllist.h"
#include "macros.h"
#include "worker.h"

static int reader_ret_val = EXIT_FAILURE;

void *reader_worker(void *arg) {
  worker_args_t *worker_args = (worker_args_t *)arg;
  const char *input_file = worker_args->extra;

  reader_ret_val = EXIT_FAILURE;

  lllist_t list = worker_args->list;

  FILE *stream;
  char *line = NULL;
  size_t len = 0;
  ssize_t nread;

  stream = fopen(input_file, "r");
  if (stream == NULL) {
    perror("Open INPUT_FILE");
    pthread_exit(&reader_ret_val);
  }

  while ((nread = getline(&line, &len, stream)) != -1) {
    // `line[nread-1]` is a '\0' or '\n'
    // the '\0' is not counted in `nread` if it was not input
    line[nread - 1] = '\0';

    if (lllist_node_add(&list, line, nread) != LLLIST_SUCCESS) {
      perror("sbuffer_insert");
      goto reader_cleanup;
    }

    // TODO: pthread_cond_broadcast
  }

  if (errno != 0) {
    perror("getline");
  } else {
    reader_ret_val = EXIT_SUCCESS;
  }

  // Clean up
reader_cleanup:
  lllist_end(&list);

  if (line != NULL) {
    free(line);
  }
  fclose(stream);

#if LOG_LVL >= INFO
  fprintf(stderr, "INFO(reader_worker): list ended with node: %p\n",
          (void *)list.first);
  fprintf(stderr, "INFO(reader_worker): exit with code %d\n", reader_ret_val);
#endif

  pthread_exit(&reader_ret_val);
}
