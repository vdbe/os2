#include "logger.h"

#include <errno.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

static bool start_exit = false;
void handle_sig_term_logger(int);

noreturn void main_logger(int pipefd_read) {
  char buf[BUFSIZ];
  ssize_t msg_len;
  struct sigaction sa;
  int ret;
  size_t line_count;
  FILE *fptr_logfile;

  ret = EXIT_FAILURE;
  line_count = 0;

  if (sigemptyset(&sa.sa_mask) != 0) {
    perror("logger sigemptyset");
    goto logger_close_pipe;
  }

  sa.sa_flags = SA_RESTART;
  sa.sa_handler = handle_sig_term_logger;
  if (sigaction(SIGTERM, &sa, NULL) != 0) {
    perror("logger sigaction");
    goto logger_close_pipe;
  }

  fptr_logfile = fopen("log.txt", "w");
  if (fptr_logfile == NULL) {
    perror("logger fopen");
    goto logger_cleanup;
  }

  while (!start_exit) {
    // TODO: Read until newline or null byte
    // Current method joins lines if writes happen _very_ close together
    msg_len = read(pipefd_read, buf, BUFSIZ);
    if (msg_len < 0) {
      if (errno == EINTR)
        continue;

      perror("logger read");
      goto logger_close_pipe;
    }

    if (msg_len == 0) {
      break;
    }

    // NOTE: Output will get weird if it's terminated by a newline or null byte
    line_count += buf[msg_len - 1] == '\n' || buf[msg_len - 1] == '\0';

    fwrite(buf, 1, msg_len, stdout);

    fprintf(fptr_logfile, "%03ld ", line_count);
    fwrite(buf, 1, msg_len, fptr_logfile);
    fflush(fptr_logfile);
  }

  ret = EXIT_SUCCESS;

logger_cleanup:
  if (fclose(fptr_logfile) != 0)
    perror("logger close log file");

logger_close_pipe:
  if (close(pipefd_read) != 0)
    perror("logger close pipefd_read");
  exit(ret);
}

void handle_sig_term_logger(int sig) {
  (void)sig;
  start_exit = true;
}
