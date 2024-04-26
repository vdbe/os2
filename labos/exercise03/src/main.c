#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <limits.h>

#include "sbuffer.h"
#include "reader.h"
#include "worker.h"

#define INPUT_FILE "inputs.txt"



int main(int argc, char *argv[]) {
  (void)argc;
  (void)argv;
	int ret = -1;
	void* retval = &ret;
	void** retval2 = &retval;
	char* input_file = INPUT_FILE;

	pthread_t thread_id;
	pthread_attr_t attr;
	worker_args_t worker_args = {.extra = input_file};

	if (sbuffer_init(&(worker_args.sbuffer)) != SBUFFER_SUCCESS) {
		perror("sbuffer_init");
		exit(EXIT_FAILURE);
	}
	printf("sbuffer main: %p\n", (void*)worker_args.sbuffer);
	//printf("sbuffer main: %p\n", (void*)*(void**)(worker_args.sbuffer));

	worker_args.extra = input_file;

	if (pthread_attr_init(&attr) != 0) {
		perror("pthread attr init");
		exit(EXIT_FAILURE);
	}

	if (pthread_attr_setstacksize(&attr, PTHREAD_STACK_MIN) != 0) {
		perror("pthread set stacksize");
		exit(EXIT_FAILURE);
	}

	if(pthread_create(&thread_id, &attr, &reader_worker, &worker_args) != 0) {
		perror("pthread create");
		exit(EXIT_FAILURE);
	}

	if(pthread_attr_destroy(&attr) != 0) {
		perror("pthread attr destroy");
		exit(EXIT_FAILURE);
	}

	if(pthread_join(thread_id, &retval) != 0) {
		perror("pthread join");
		exit(EXIT_FAILURE);
	}

	if(retval  == PTHREAD_CANCELED) {
		fprintf(stderr, "thread canceled\n");
	} else if (*(int*)retval){
		fprintf(stderr, "thread exited with code %d\n", **(int**)retval2);
	}

	char *data = NULL;
	size_t data_len = 0;;

	while ( sbuffer_remove(worker_args.sbuffer, &data, &data_len) == SBUFFER_SUCCESS) {
		printf("main: %s\n", data);
	}

	if(data) {
		free(data);
	}

	sbuffer_free(&worker_args.sbuffer);

  return EXIT_SUCCESS;
}
