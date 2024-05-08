#include "lllist.h"

#include <pthread.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "macros.h"

/*
 *
 * TODO: If possible replace empty node with with `NULL` ptr
 *
 */

struct lllist_node {
  struct lllist_node *next;
};

struct node {
  struct lllist_node node;
	bool arrived;
  atomic_int readers;

  size_t data_len;
  char *data;
};

inline int lllist_init(struct lllist_head *list, int readers) {
  // Inital empty `node`
  list->first = calloc(1, sizeof(struct node));
  if (list->first == NULL) {
    return LLLIST_FAILURE;
  }
	fprintf(stderr, "%d\n", __LINE__);

  list->first_unfree = malloc(sizeof(list->first_unfree));
  if (list->first_unfree == NULL) {
    free(list->first);
    return LLLIST_FAILURE;
  }

  list->mutex_new_node = malloc(sizeof(*list->mutex_new_node));
  if (list->mutex_new_node == NULL) {
    free(list->first);
		free(list->first_unfree);
    return LLLIST_FAILURE;
  }

	if(pthread_mutex_init(list->mutex_new_node, NULL) != 0) {
		perror("phtread_mut_init");
		free(list->first);
		free(list->first_unfree);
		free(list->mutex_new_node);
		return LLLIST_FAILURE;
	}

  list->cond_new_node = malloc(sizeof(*list->cond_new_node));
  if (list->cond_new_node == NULL) {
		free(list->first);
		free(list->first_unfree);
		pthread_mutex_destroy(list->mutex_new_node);
		free(list->mutex_new_node);
		return LLLIST_FAILURE;
  }

	if(pthread_cond_init(list->cond_new_node, NULL) != 0) {
		perror("phtread_cond_init");
		free(list->first);
		free(list->first_unfree);
		pthread_mutex_destroy(list->mutex_new_node);
		free(list->mutex_new_node);
		free(list->cond_new_node);
		return LLLIST_FAILURE;
	}

  *list->first_unfree = list->first;

  list->readers = readers;

#if LOG_LVL >= INFO
  fprintf(stderr, "INFO(lllist_init): initial node %p, first_unfree %p\n",
          (void *)list->first, (void *)list->first_unfree);
#endif

  return LLLIST_SUCCESS;
}

inline void lllist_end(struct lllist_head *head) {
  ((struct node *)head->first)->node.next = head->first;

	if(pthread_cond_broadcast(head->cond_new_node) != 0) {
		// TODO: No idea how to handle this error
		perror("lllist_add pthread cond broadast new node");
	}
}

inline void lllist_add(struct lllist_head *head, struct lllist_node *new) {
	bool *arrived;

	arrived = &((struct node*)head->first)->arrived;

  // Point `next` of old filled node to the new empty node
  head->first->next = new;

  // Point `next` to the new node
  head->first = new;

	if(*arrived == true) {
#if LOG_LVL >= INFO
  fprintf(stderr, "=================INFO(lllist_add): broadcast new node\n");
#endif
		if(pthread_cond_broadcast(head->cond_new_node) != 0) {
			// TODO: No idea how to handle this error
			perror("lllist_add pthread cond broadast new node");
		}
	}
}

inline int lllist_free(struct lllist_head *list) {
  struct node *node;
  struct node *node_next;

  node = *(struct node **)list->first_unfree;

#if LOG_LVL >= DEBUG
  fprintf(stderr, "INFO(lllist_end): list->first_unfree: %p\n", (void *)node);
#endif

  while (node->node.next == NULL) {
    node_next = (struct node *)node->node.next;

#if LOG_LVL >= INFO
    fprintf(stderr, "INFO(lllist_free): free node %p next %p\n", (void *)node,
            (void *)node_next);
#endif

    free(node);

    if (node == node_next) {
      break;
    }
    node = node_next;
  }
  list->first = NULL;

#if LOG_LVL >= INFO
  fprintf(stderr, "INFO(lllist_free): free list->first_unfree: %p\n",
          (void *)list->first_unfree);
#endif
  free(list->first_unfree);

	if (pthread_mutex_destroy(list->mutex_new_node) != 0) {
		perror("mutex destroy");
		return LLLIST_FAILURE;
	}

	free(list->mutex_new_node);

	if (pthread_mutex_destroy(list->mutex_new_node) != 0) {
		perror("cond destroy");
		return LLLIST_FAILURE;
	}

	free(list->cond_new_node);

  return LLLIST_SUCCESS;
}

inline void free_node(struct node *node) {
  free(node->data);
  free(node);
}

inline int lllist_node_add(struct lllist_head *head, char *data,
                           size_t data_len) {
  struct node *node;
  node = (struct node *)head->first;

// Sanity check
// `head->first` should always be empty for the reader at the start of
// `node_add`, except when everything is read then it should point to itself
// and we don't add another node
#if LOG_LVL >= WARN
  if (!(node->node.next == NULL && node->readers == 0 && node->data == NULL &&
        node->data_len == 0)) {
    fprintf(stderr,
            "WARN(lllist_node_add): Something has gone seriously wrong!!!\n");
    fprintf(stderr,
            "node:\n"
            "\tnode.next: %p\n"
            "\treader: %d\n"
            "\tdata: %p\n"
            "\tdata_len: %ld\n",
            (void *)node->node.next, node->readers, (void *)node->data,
            node->data_len);

    return LLLIST_FAILURE;
  }
#endif

  // Populate head with the data from `data`
  node->data = calloc(data_len, sizeof(*node->data));
  if (node->data == NULL) {
    return LLLIST_FAILURE;
  }
  memcpy(node->data, data, data_len);
  node->data_len = data_len;
  // node->readers = 0;

  // From here `node` is the empty leading node

  // Allocate new empty head
  node = calloc(1, sizeof(*node));
  if (node == NULL) {
    return LLLIST_FAILURE;
  }

  // Point filled and head to new empty node
  lllist_add(head, (struct lllist_node *)node);

#if LOG_LVL >= INFO
  fprintf(stderr, "INFO(lllist_node_add): added node %p\n", (void *)node);
#endif

  return LLLIST_SUCCESS;
}

inline int lllist_node_consume(struct lllist_head *head, char **data, 
																size_t *data_len, size_t *data_buf_len) {
  struct node *node;
  node = (struct node *)head->first;
  int readers;

  TSAN_ANNOTE_BENIGN_RACE_SIZED(&node->node.arrived, sizeof(node->node.arrived),
                                "Race node->node.next");
	node->arrived = true;

  // _Last_ write to `node` by `node_add` is change `node->node.next` from
  // `NULL` to the ptr of the next node.
  // If it's not NULL it's safe to _read_ without race conditions
  TSAN_ANNOTE_BENIGN_RACE_SIZED(&node->node.next, sizeof(node->node.next),
                                "Race node->node.next");
  // if (node->node.next == NULL) {
  //   return LLLIST_NO_DATA;
  // }
	while (node->node.next == NULL) {
#if LOG_LVL >= INFO
    fprintf(stderr, "===== INFO(lllist_node_consume): waiting for new node %p\n",
            (void*)node);
#endif
		if(pthread_cond_wait(head->cond_new_node, head->mutex_new_node) != 0) {
			// TODO: No idea how to handle this error
			perror("lllist_add pthread cond wait new node");
			return LLLIST_NO_DATA;
		};
	}

  // If `head->first`/`node` points to itself the reader has finished
  if (node->node.next == head->first) {
    // TODO: Better return value
    return LLLIST_ENDED;
  }

  // Resize `data` to fit `node->data` if needed
  TSAN_ANNOTE_BENIGN_RACE_SIZED(&node->data_len, sizeof(node->data_len),
                                "Race node->data_len");
  if (*data_buf_len < node->data_len) {
    char *tmp;

#if LOG_LVL >= INFO
    fprintf(stderr, "INFO(lllist_node_consume): growing data from %ld to %ld\n",
            *data_buf_len, node->data_len);
#endif

    tmp = realloc(*data, node->data_len * sizeof(**data));
    if (tmp == NULL) {
      return LLLIST_FAILURE;
    }

    *data = tmp;
    *data_buf_len = node->data_len;
  }

  // NOTE: This is not a race condition since `node->node.next` is not `NULL`
  // meaning that `node->data` and `node->data_len` are alreade set
  TSAN_ANNOTE_BENIGN_RACE_SIZED(&node->data, sizeof(node->data),
                                "Race on node->data");
  TSAN_ANNOTE_BENIGN_RACE_SIZED(node->data, node->data_len,
                                "Race on *node->data");
  // TODO: Doesn't say it errors but maybe it returns `NULL`
  memcpy(*data, node->data, node->data_len);
	*data_len = node->data_len;

  head->first = node->node.next;

  // Reduce reader by 1 and get reader count
  TSAN_ANNOTE_BENIGN_RACE_SIZED(&node->readers, sizeof(node->readers),
                                "Race node->readers");
  { // CRITICAL: start
    readers = atomic_fetch_add(&node->readers, 1);
  } // CRITICAL: end

  // `readers + 1` since readers is the value before the addition
  if ((readers + 1) == head->readers) {
#if LOG_LVL >= INFO
    fprintf(stderr, "INFO(lllist_node_consume): free node %p readers %d\n",
            (void *)node, readers + 1);
#endif

    // NOTE: This does not need to be in the criticial section since
    // - `head->first_unfree` is only used in `lllist_free` which may only be
    //    called once all readers and writers have exited.
    // - We are the trailing reader, no other thread can free the next node
    //    and write to `*head->frist_unfree` before we comsume it.
    *head->first_unfree = head->first;

    free_node(node);
  }

  return LLLIST_SUCCESS;
}
