#include "server.h"

#include <arpa/inet.h>
#include <assert.h>
#include <errno.h>
#include <signal.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

int setup_server_sock(void);
int server(int);
noreturn void handle_connection(int, struct sockaddr_in6);

void handle_sig_term_server(int);
void handle_sig_child_server(int);
void handle_sig_connection(int);

static bool start_exit = false;
static FILE *loggerfd;

noreturn void main_server(int pipefd_write) {
  int server_sock_fd, ret;
  ret = EXIT_FAILURE;

  {
    // NOTE: Check goto's interaction with exiting scope
    struct sigaction sa;

    if (sigemptyset(&sa.sa_mask) != 0) {
      perror("server sigemptyset");
      goto main_server_cleanup;
    }

    sa.sa_flags = 0; // No `SA_RESTART` since we want to exit
    sa.sa_handler = handle_sig_term_server;
    if (sigaction(SIGTERM, &sa, NULL) != 0) {
      perror("server sigaction SIGTERM");
      goto main_server_cleanup;
    }

    sa.sa_flags = SA_RESTART;
    sa.sa_handler = handle_sig_child_server;
    if (sigaction(SIGCHLD, &sa, NULL) != 0) {
      perror("server sigaction SIGCHLD");
      goto main_server_cleanup;
    }

    // Ignore inherited signal hanlders
    sa.sa_handler = SIG_IGN;
    if (sigaction(SIGUSR1, &sa, NULL) != 0 ||
        sigaction(SIGINT, &sa, NULL) != 0) {
      perror("server sigaction ignore");
      goto main_server_cleanup;
    }
  }

  // Reset groupid to pid, this is to seperate logging from server
  // so we can signal all children
  if (setpgrp() != 0) {
    perror("server setgrp");
    goto main_server_cleanup;
  }

  server_sock_fd = setup_server_sock();
  if (server_sock_fd < 0) {
    goto main_server_cleanup;
  }

  loggerfd = fdopen(pipefd_write, "w");
  if (loggerfd == NULL) {
    perror("server fdopen");
    goto main_server_cleanup;
  }

  if (server(server_sock_fd) != 0) {
    goto main_server_cleanup;
  }

  ret = EXIT_SUCCESS;

main_server_cleanup:
  // `fdopen` doesn't duplicate the fd, so `close(pipefd_write)` is not needed
  if (fclose(loggerfd) != 0)
    perror("server close write file handle");

  exit(ret);
}

int setup_server_sock(void) {
  int server_sock_fd;

  struct sockaddr_in6 server_addr;
  const int on = 1;

  // sockfd = socket(AF_INET6, SOCK_STREAM, IPPROTO_TCP);
  server_sock_fd = socket(AF_INET6, SOCK_STREAM, 0);
  if (server_sock_fd == -1) {
    perror("server socket");
    return -1;
  }

  if (setsockopt(server_sock_fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) !=
      0) {
    perror("server setsockopt");
    if (close(server_sock_fd) != 0)
      perror("server close socket_fd");
    return -1;
  }

  memset(&server_addr, 0, sizeof(server_addr));
  server_addr.sin6_family = AF_INET6;
  server_addr.sin6_port = htons(PORT);
  server_addr.sin6_addr = in6addr_any; // `::`

  if (bind(server_sock_fd, (struct sockaddr *)&server_addr,
           sizeof(server_addr)) != 0) {
    perror("server bind");
    if (close(server_sock_fd) != 0)
      perror("server close socket_fd");
    return -1;
  }

  if (listen(server_sock_fd, LISTEN_BACKLOG) != 0) {
    perror("server listen");
    if (close(server_sock_fd) != 0)
      perror("server close socket_fd");
    return -1;
  }

  return server_sock_fd;
}

int server(int server_sock_fd) {
  struct sockaddr_in6 client_addr;
  int client_sock_fd;
  const socklen_t sockaddr_len = sizeof(client_addr);
  pid_t child_pid;

  fprintf(loggerfd, "Server started, accepting connections\n");
  fflush(loggerfd);

  while (!start_exit) {
    client_sock_fd = accept(server_sock_fd, (struct sockaddr *)&client_addr,
                            (socklen_t *)&sockaddr_len);
    if (client_sock_fd <= -1) {
      if (errno != EINTR)
        perror("server accept");

      // Can be a `break` since all sig actions without the `SA_RESTART` flag
      // set are actions that increment `start_exit`
      continue;
    }

    child_pid = fork();
    switch (child_pid) {
    case -1: // Error
      perror("server fork");
      break;
    case 0: { // (connection) Child
      struct sigaction sa;

      if (sigemptyset(&sa.sa_mask) != 0) {
        perror("connnection sigemptyset");
        return -1;
      }

      sa.sa_flags = 0; // No `SA_RESTART` since we want to exit
      sa.sa_handler = handle_sig_connection;
      if (sigaction(SIGUSR1, &sa, NULL) != 0 ||
          sigaction(SIGTERM, &sa, NULL) != 0) {
        perror("connection sigaction SIGTERM/SIGCHLD");
        return -1;
      }

      // Ignore inherited signal hanlders
      sa.sa_handler = SIG_IGN;
      if (sigaction(SIGCHLD, &sa, NULL) != 0) {
        perror("connection sigaction ignore");
        return -1;
      }

      if (close(server_sock_fd) != 0)
        perror("client connection close server_sock_fd");

      handle_connection(client_sock_fd, client_addr);
      assert(!"Should never get here");
    }
    default:; // (server) Parent
    }

    if (close(client_sock_fd) != 0)
      perror("server close client_sock_fd");

    fprintf(loggerfd, "client connected, child forked with pid %d\n",
            child_pid);
    fflush(loggerfd);
  }

  if (close(server_sock_fd) != 0) {
    perror("server close server_sock_fd");
    return -1;
  }

  // Wait for all children
  // TODO: Error handling
  while (wait(NULL) > 0)
    ;

  fprintf(loggerfd, "no more children, shutting down server\n");
  fflush(loggerfd);

  return 0;
}

noreturn void handle_connection(int client_sock_fd,
                                struct sockaddr_in6 client_addr) {
  const char *goodbye_msg = "goodbye";
  char client_ip[INET6_ADDRSTRLEN];
  char buf[BUFSIZ];
  ssize_t msg_len;
  int ret = EXIT_FAILURE;

  inet_ntop(AF_INET6, &client_addr.sin6_addr, client_ip, INET6_ADDRSTRLEN);

  while (!start_exit) {
    msg_len = recv(client_sock_fd, buf, BUFSIZ, 0);
    if (msg_len == 0) {
      // Connection closed from clientside
      break;
    } else if (msg_len < 0) {
      if (errno == EINTR)
        continue;

      perror("client recv");
      goto handle_connection_cleanup;
    }

    fprintf(loggerfd, "received: ");
    fwrite(buf, 1, msg_len, loggerfd);
    fflush(loggerfd);

    // NOTE: How does this react to an interupt?
    if (send(client_sock_fd, buf, msg_len, 0) != msg_len) {
      perror("send");
      goto handle_connection_cleanup;
    };
  }

  fprintf(loggerfd, "client closed connection\n");
  fflush(loggerfd);

  if (send(client_sock_fd, goodbye_msg, strlen(goodbye_msg), 0) !=
      (ssize_t)strlen(goodbye_msg)) {
    perror("send goodbye");
    goto handle_connection_cleanup;
  }

  ret = EXIT_SUCCESS;

handle_connection_cleanup:
  if (close(client_sock_fd) != 0) {
    perror("client close client_sock_fd");
  }

  exit(ret);
}

void handle_sig_term_server(int sig) {
  (void)sig;
  // sig:
  //  - SIGTERM
  pid_t gpid, prev_errno;
  prev_errno = errno;

  fprintf(loggerfd, "server received SIGTERM, closing child connections\n");
  fflush(loggerfd);

  start_exit = true;

  gpid = getpgrp();
  if (gpid < 0) {
    perror("handle_sig_term_server getpgrp");
  } else {
    kill(-gpid, SIGUSR1);
  }

  errno = prev_errno;
}

void handle_sig_child_server(int sig) {
  (void)sig;
  // sig:
  //  - SIGCHLD

  int prev_errno;
  prev_errno = errno;

  pid_t gpid = getpgrp();
  if (gpid < 0) {
    perror("handle_sig_child_server getpgrp");
  } else {
    // TODO: Handle child errors
    while (waitpid(-gpid, NULL, WNOHANG) > 0)
      ;
  }

  errno = prev_errno;
}

void handle_sig_connection(int sig) {
  (void)sig;
  // sig:
  //  - SIGTERM
  //  - SIGUSR1

  start_exit = true;
}
