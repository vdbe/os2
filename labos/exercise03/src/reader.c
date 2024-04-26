#include "reader.h"

#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#include "worker.h"

static int reader_retval = EXIT_FAILURE;

void* reader_worker(void *arg) {
	worker_args_t* worker_args = (worker_args_t*)arg;
	const char* input_file = worker_args->extra;
	sbuffer_t* sbuffer = worker_args->sbuffer;

	printf("sbuffer reader: %p\n", (void*)sbuffer);
	//printf("sbuffer reader: %p\n", (void*)*(void**)sbuffer);

	FILE *stream;
	char *line = NULL;
	size_t len = 0;
	ssize_t nread;

	stream = fopen(input_file, "r");
	if (stream == NULL) {
		perror("Open INPUT_FILE");
		pthread_exit(&reader_retval);
	}

	while ((nread = getline(&line, &len, stream)) != -1) {
		fprintf(stderr, "%ld: ", nread);
		fwrite(line, nread, 1, stdout);

		// `line[nread-1]` is a '\0' or '\n'
		// the '\0' is not counted in `nread` if it was not input
		line[nread-1] = '\0';

		if(sbuffer_insert(sbuffer, line, nread) != SBUFFER_SUCCESS) {
			perror("sbuffer_insert");
			goto reader_cleanup;
		}
		// TODO: pthread_cond_broadcast
	}

	if (errno != 0) {
		perror("getline");
	} else {
		printf("SUCCESS\n");
		reader_retval = EXIT_SUCCESS;
	}

	// Clean up
reader_cleanup:
	if(line != NULL) {
		free(line);
	}
	fclose(stream);


	pthread_exit(&reader_retval);
}
