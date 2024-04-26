/**
 * @author {AUTHOR}
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "sbuffer.h"

/**
 * basic node for the buffer, these nodes are linked together to create the buffer
 */
typedef struct sbuffer_node {
	struct sbuffer_node *next;  /**< a pointer to the next node*/
	// char data[MESSAGE_MAX];         /**< a string with MESSAGE_MAX length */
	char *data;         /**< a string with MESSAGE_MAX length */
	size_t data_len;
	size_t reads_until_free; /** Amount of reads before free */
} sbuffer_node_t;


/**
 * a structure to keep track of the buffer
 */
struct sbuffer {
	sbuffer_node_t *head;       /**< a pointer to the first node in the buffer */
	sbuffer_node_t *tail;       /**< a pointer to the last node in the buffer */
};

int sbuffer_node_free(sbuffer_node_t* sbuffer_node);

int sbuffer_init(sbuffer_t ** buffer) {
	*buffer = malloc(sizeof(sbuffer_t));
	if (*buffer == NULL) return SBUFFER_FAILURE;
	(*buffer)->head = NULL;
	(*buffer)->tail = NULL;
	return SBUFFER_SUCCESS;
}

// CRITICAL: function
int sbuffer_free(sbuffer_t ** buffer) {
	sbuffer_node_t *dummy;
	if ((buffer == NULL) || (*buffer == NULL)) {
		return SBUFFER_FAILURE;
	}
	while ((*buffer)->head) {
		dummy = (*buffer)->head;
		(*buffer)->head = (*buffer)->head->next;
		sbuffer_node_free(dummy);
	}
	free(*buffer);
	*buffer = NULL;
	return SBUFFER_SUCCESS;
}

int sbuffer_remove(sbuffer_t * buffer, char **data, size_t* data_len) {
	sbuffer_node_t *dummy;
	if (buffer == NULL)
		return SBUFFER_FAILURE;

	if (buffer->head == NULL) {
		// TODO: pthread_cond_wait when this is returned
		return SBUFFER_NO_DATA;
	}

	// Expand passed in data buffer if it's not large enough
	if (buffer->head->data_len > *data_len) {
		fprintf(stderr, "Realloc from %ld to %ld\n", *data_len, buffer->head->data_len);
		char* tmp = realloc(*data, sizeof(char*)*buffer->head->data_len);
		if (tmp == NULL) {
			return SBUFFER_FAILURE;
		}
		*data = tmp;
		*data_len = buffer->head->data_len;
	}

	strncpy(*data, buffer->head->data, buffer->head->data_len);
	dummy = buffer->head;

	{ // CRITICAL: Start
		if (buffer->head == buffer->tail) // buffer has only one node
		{
			buffer->head = buffer->tail = NULL;
		} else  // buffer has many nodes empty
		{
			buffer->head = buffer->head->next;
		}
		dummy->reads_until_free -= 1;
		sbuffer_node_free(dummy);
	} // CRITICAL: End
	return SBUFFER_SUCCESS;
}

int sbuffer_insert(sbuffer_t * buffer, char * data, size_t n) { 
	sbuffer_node_t *dummy;

	if (buffer == NULL)
		return SBUFFER_FAILURE;

	dummy = malloc(sizeof(sbuffer_node_t));
	if (dummy == NULL)
		return SBUFFER_FAILURE;

	// Setup dummy
	dummy->data = strndup(data, n);
	dummy->reads_until_free = 2; // digest, freq_table
	dummy->data_len = n;
	dummy->next = NULL;

	{ // CRITICAL: Start
		if (buffer->tail == NULL) {
			// buffer empty (buffer->head should also be NULL)
			buffer->head = buffer->tail = dummy;
		} else {
			// buffer not empty
			buffer->tail->next = dummy;
			buffer->tail = buffer->tail->next;
		}
	} // CRIRICAL: end


	return SBUFFER_SUCCESS;
}

int sbuffer_node_free(sbuffer_node_t* sbuffer_node) {
	free(sbuffer_node->data);
	free(sbuffer_node);

	return SBUFFER_SUCCESS;
}
