/*
 * echoserveri.c - An iterative echo server
 */
/* $begin echoserverimain */
#include "command.h"
#include "csapp.h"
#include "misc.h"
#include "stock.h"

#define MAX_CONNECTION 256

int main(int argc, char **argv) {
  /* Enough space for any address */ // line:netp:echoserveri:sockaddrstorage
  char client_hostname[MAXLINE], client_port[MAXLINE];
  int listenfd, connfd, maxfd, active_client_len = 0;
  struct sockaddr_storage client_addr;
  socklen_t client_len;
  fd_set pending_fds, pending_mask;

  if (argc != 2) {
    fprintf(stderr, "usage: %s <port>\n", argv[0]);
    exit(0);
  }

  stock_init();

  maxfd = listenfd = Open_listenfd(argv[1]);
  debug_print("now listening...");

  FD_ZERO(&pending_mask);
  FD_SET(listenfd, &pending_mask);
  debug_print("initialised fd mask sets");

  while (1) {
    debug_print("select maxfd=%d, active clients=%d", maxfd, active_client_len);
    pending_fds = pending_mask;
    Select(maxfd + 1, &pending_fds, NULL, NULL, NULL);

    for (int fd = 0; fd <= maxfd; fd++) {
      debug_print("looking into fd %d", fd);
      if (FD_ISSET(fd, &pending_fds)) {
        if (fd == listenfd) {
          // new connection is being established
          debug_print("incoming new connection");
          client_len = sizeof(struct sockaddr_storage);
          connfd = Accept(listenfd, (SA *)&client_addr, &client_len);
          Getnameinfo((SA *)&client_addr, client_len, client_hostname, MAXLINE,
                      client_port, MAXLINE, 0);
          printf("Connected to (%s, %s)\n", client_hostname, client_port);

          FD_SET(connfd, &pending_mask);
          active_client_len++;
          if (maxfd < connfd) {
            maxfd = connfd;
          }
        } else {
          debug_print("update on existing connection fd %d", fd);
          // handle connection update and manage stock table
          if (handle_connection(fd) == COMMAND_EXIT) {
            FD_CLR(fd, &pending_mask);
            Close(fd);
            active_client_len--;
            debug_print("removed fd %d from active client list", fd);

            // write stock data only when there is no active client left
            if (active_client_len == 0) {
              stock_write();
            }
          }
        }
      }
    }
  }
  exit(0);
}
/* $end echoserverimain */