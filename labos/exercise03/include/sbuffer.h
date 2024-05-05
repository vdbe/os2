/**
 * @author {AUTHOR}
 */

#ifndef _SBUFFER_H_
#define _SBUFFER_H_

#include <stddef.h>

#define SBUFFER_FAILURE -1
#define SBUFFER_SUCCESS 0
#define SBUFFER_NO_DATA 1

#define MESSAGE_MAX 512

typedef struct sbuffer sbuffer_t;

/**
 * Allocates and initializes a new shared buffer
 * @param buffer a double pointer to the buffer that needs to be initialized
 * @return SBUFFER_SUCCESS on success and SBUFFER_FAILURE if an error occurred
 */
int sbuffer_init(sbuffer_t **buffer);

/**
 * All allocated resources are freed and cleaned up
 * @param buffer a double pointer to the buffer that needs to be freed
 * @return SBUFFER_SUCCESS on success and SBUFFER_FAILURE if an error occurred
 */
int sbuffer_free(sbuffer_t **buffer);

/**
 * Removes the first sensor data in 'buffer' (at the 'head') and returns this
 * element data as '*data' If 'buffer' is empty, the function doesn't block
 * until new element become available but returns SBUFFER_NO_DATA
 * @param buffer a pointer to the buffer that is used
 * @param data a pointer to pre-allocated char* space (512 bytes), the data will
 * be copied into this structure. No new memory is allocated for 'data' in this
 * function.
 * @return SBUFFER_SUCCESS on success and SBUFFER_FAILURE if an error occurred
 */
int sbuffer_remove(sbuffer_t *buffer, char **data, size_t *data_len);

/**
 * Inserts the data in 'data' at the end of 'buffer' (at the 'tail')
 * @param buffer a pointer to the buffer that is used
 * @param data a pointer to sbuffer_element_t data, that will be copied into the
 * buffer
 * @param n length of the data that will be copied
 * @return SBUFFER_SUCCESS on success and SBUFFER_FAILURE if an error occurred
 */
int sbuffer_insert(sbuffer_t *buffer, char *data, size_t n);

#endif //_SBUFFER_H_
