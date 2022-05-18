/*
 * echoserveri.c - An iterative echo server
 */
#define DEBUG

/* $begin echoserverimain */
#include "csapp.h"
#include "misc.h"

void echo(int connfd);

void handle_connection(int connfd);
char *handle_cmd(char *args[], int length);
size_t parse(char *cmd, char **buf);

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
    int plen;
    char *response, *pbuf[MAX_COMMAND_ARGS];

    debug_print("server received %d bytes", n);

    plen = parse(rtrim(buf), pbuf);
    for (int i = 0; i < plen; i++) {
      debug_print("pbuf[%d] = \"%s\"", i, pbuf[i]);
    }

    if (response = handle_cmd(pbuf, plen)) {
      Rio_writen(connfd, response, strlen(response) + 1);
      free(response);
    } else {
    }
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

/**
 * @brief Execute by reading from command string list.
 * @warning Returned string must be freed after use.
 *
 * @param args Argument list for command string.
 * @param length Length for @p args.
 * @return Pointer to response string. NULL if client requested termination.
 */
char *handle_cmd(char *args[], int length) {
  debug_print("handling command %s...", args[0]);
  char *response = malloc(sizeof(char) * MAXLINE);

  if (!strcmp(args[0], "exit")) {
    // client requested termination
    free(response);
    return NULL;
  } else if (!strcmp(args[0], "show")) {
    // show current stock status
    char *status = fetch_stock_stat();
    strcpy(response, status);
    free(status);
  } else if (!strcmp(args[0], "buy")) {
    // remove item from stock
  } else if (!strcmp(args[0], "sell")) {
    // add item to stock
  } else {
    debug_print("invalid command \"%s\"", args[0]);
    strcpy(response, "invalid command");
  }
  return response;
}