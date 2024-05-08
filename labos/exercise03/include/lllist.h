#pragma once

#include <stdlib.h>

#define LLLIST_SUCCESS 0
#define LLLIST_NO_DATA 1
#define LLLIST_ENDED 2
#define LLLIST_FAILURE -1

struct lllist_head {
  struct lllist_node *first;
  struct lllist_node **first_unfree;
  int readers;

	pthread_cond_t *cond_new_node;
	pthread_mutex_t *mutex_new_node;
};

typedef struct node node_t;
typedef struct lllist_head lllist_t;

int lllist_init(lllist_t *, int);
void lllist_end(lllist_t *);
int lllist_free(lllist_t *);

int lllist_node_add(lllist_t *, char *, size_t);
int lllist_node_consume(lllist_t *, char **, size_t *, size_t *);
