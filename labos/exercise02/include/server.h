#pragma once

#include <stdnoreturn.h>

#define PORT 9001
#define LISTEN_BACKLOG 1 << 4

noreturn void main_server(int);
