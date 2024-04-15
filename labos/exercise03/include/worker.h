#pragma once

#include "sbuffer.h"

typedef struct {
	sbuffer_t* sbuffer;

	// Worker specific data
	void* extra;
} worker_args_t;
