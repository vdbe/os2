#define DEBUG 1

#include <assert.h>
#include <errno.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#include "logger.h"
#include "server.h"

void handle_sig(int);
void wait_for_processes(void);
void wait_for_pid(pid_t *);

static pid_t logger, server;

int main(int argc, char *argv[]) {
  (void)argc;
  (void)argv;
  int ret;
  union {
    int raw[2];
    struct {
      int read;
      int write;
    };
  } pipefd;
  struct sigaction sa;
  ret = EXIT_FAILURE;

  if (sigemptyset(&sa.sa_mask) != 0) {
    perror("main sigemptyset");
    goto main_ret;
  }
  sa.sa_flags = SA_RESTART;
  sa.sa_handler = handle_sig;
  if (sigaction(SIGINT, &sa, NULL) != 0 || sigaction(SIGTERM, &sa, NULL) != 0 ||
      sigaction(SIGCHLD, &sa, NULL) != 0) {
    perror("main sigaction");
    goto main_ret;
  }

  if (pipe(pipefd.raw) != 0) {
    perror("pipe");
    goto main_ret;
  }

  logger = fork();
  switch (logger) {
  case -1: // Error
    perror("main fork logger");
    handle_sig(SIGTERM);
    goto main_cleanup;
    break;
  case 0: // (logger) Child
    if (close(pipefd.write))
      perror("logger close pipe write");

    main_logger(pipefd.read);
    assert(!"Should never get here");
  default:; // Parent
  }

  server = fork();
  switch (server) {
  case -1: // Error
    perror("main fork server");
    handle_sig(SIGTERM);
    goto main_cleanup;
    break;
  case 0: // (server) Child
    if (close(pipefd.read))
      perror("server close pipe read");
    main_server(pipefd.write);
    assert(!"Should never get here");
  default:; // Parent
  }

  ret = EXIT_SUCCESS;

main_cleanup:
  // TODO: handle/print errors
  close(pipefd.read);
  close(pipefd.write);

  wait_for_processes();

main_ret:
  return ret;
}

void wait_for_pid(pid_t *pid) {
  // TODO: Handle errors
  while (*pid != 0 && waitpid(*pid, NULL, 0) != *pid) {
    switch (errno) {
    case EINTR:
      continue;
    case ECHILD:
      break;
    default:
      // NOTE: Make sure this does not cause an infinite loop
      perror("waitpid");
      continue;
    }
    break;
  }

  *pid = 0;
}

void wait_for_processes(void) {
  // Server exits before logger
  if (server != 0) {
    wait_for_pid(&server);
  }

  // Logger will exit once no more write handle exists to the pipe
  if (logger != 0) {
    kill(logger, SIGTERM); // Should not be needed but better safe than sorry
    wait_for_pid(&logger);
  }
}

void handle_sig(int sig) {
  (void)sig;
  int prev_errno;

  prev_errno = errno;
  if (server != 0) {
    kill(server, SIGTERM);
  }

  // Logger will start to exit once server closes it's write handle to the log
  // pipe

  errno = prev_errno;
}
