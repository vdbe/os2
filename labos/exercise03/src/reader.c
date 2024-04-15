#include "reader.h"

#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#include "worker.h"

static char*test = "test123\0";

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
	int ret = EXIT_FAILURE;

	stream = fopen(input_file, "r");
	if (stream == NULL) {
		perror("Open INPUT_FILE");
		pthread_exit(&ret);
	}

	while ((nread = getline(&line, &len, stream)) != -1) {
		fwrite(line, nread, 1, stdout);
		//if (nread > MESSAGE_MAX) {
		//	line[MESSAGE_MAX-1] = '\0';
		//}

		if(sbuffer_insert(sbuffer, test) != SBUFFER_SUCCESS) {
			perror("sbuffer_insert");
			goto reader_cleanup;
		}
	}

	if (errno != 0) {
		perror("getline");
	} else {
		ret = EXIT_SUCCESS;
	}

	// Clean up
reader_cleanup:
	if(line != NULL) {
		free(line);
	}
	fclose(stream);


	char data[MESSAGE_MAX];

	while ( sbuffer_remove(sbuffer, data) == SBUFFER_SUCCESS) {
		printf("reader: %s\n", data);
		printf("tail: %p, head %p\n", (void*)(sbuffer->tail), (void*)(sbuffer->head));
	}

	pthread_exit(&ret);

	return NULL;
}
