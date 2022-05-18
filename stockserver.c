/*
 * echoserveri.c - An iterative echo server
 */
#define DEBUG

/* $begin echoserverimain */
#include "csapp.h"
#include "misc.h"

void echo(int connfd);

size_t parse(char *cmd, char **buf);
void handle_connection(int connfd);

int main(int argc, char **argv) {
  int listenfd, connfd;
  socklen_t client_len;
  struct sockaddr_storage client_addr;
  /* Enough space for any address */ // line:netp:echoserveri:sockaddrstorage
  char client_hostname[MAXLINE], client_port[MAXLINE];

  if (argc != 2) {
    fprintf(stderr, "usage: %s <port>\n", argv[0]);
    exit(0);
  }

  listenfd = Open_listenfd(argv[1]);
  debug_print("now listening...");
  while (1) {
    client_len = sizeof(struct sockaddr_storage);
    connfd = Accept(listenfd, (SA *)&client_addr, &client_len);
    Getnameinfo((SA *)&client_addr, client_len, client_hostname, MAXLINE,
                client_port, MAXLINE, 0);
    printf("Connected to (%s, %s)\n", client_hostname, client_port);
    handle_connection(connfd);
    // echo(connfd);
    Close(connfd);
  }
  exit(0);
}
/* $end echoserverimain */

/**
 * @brief Handle connection for @p connfd connection
 *
 * @param connfd File descriptor for connection
 */
void handle_connection(int connfd) {
  int n;
  char buf[MAXLINE];
  rio_t rio;

  Rio_readinitb(&rio, connfd);
  while ((n = Rio_readlineb(&rio, buf, MAXLINE)) != 0) {
    int argc;
    char *cmds[MAX_COMMAND_ARGS];
    debug_print("server received %d bytes", n);
    argc = parse(rtrim(buf), cmds);
    for (int i = 0; i < argc; i++) {
      debug_print("cmds[%d] = \"%s\"", i, cmds[i]);
    }
    Rio_writen(connfd, buf, strlen(buf) + 1);
  }
}

/**
 * @brief Parse given command string into blocks.
 *
 * @warning Execution is destructive for argument @p cmd.
 *
 * @param cmd Null-terminated command string
 * @param buf Buffer to store pointers for each argument
 * @return Number of parsed command arguments
 */
size_t parse(char *cmd, char **buf) {
  int argc = 0;
  char *__next;
  debug_print("parsing command \"%s\"", cmd);

  for (char *ptr = strtok_r(cmd, DELIM_CHARS, &__next); ptr != NULL;
       ptr = strtok_r(NULL, " ", &__next)) {
    debug_print("parsing on \"%s\"", ptr);
    buf[argc++] = ptr;
  }
  return argc;
}