/*
 * echoserveri.c - An iterative echo server
 */
/* $begin echoserverimain */
#include "command.h"
#include "csapp.h"
#include "misc.h"
#include "sbuf.h"
#include "stock.h"

#define MAX_CONNECTIONS 256

void *thread(void *vargp);

static sem_t client_len_mutex;
volatile int active_client_len = 0;
sbuf_t sbuf;

int main(int argc, char **argv) {
  /* Enough space for any address */ // line:netp:echoserveri:sockaddrstorage
  char client_hostname[MAXLINE], client_port[MAXLINE];
  int listenfd, connfd;
  struct sockaddr_storage client_addr;
  socklen_t client_len;
  pthread_t tid;

  if (argc != 2) {
    fprintf(stderr, "usage: %s <port>\n", argv[0]);
    exit(0);
  }

  stock_init();
  sbuf_init(&sbuf, SBUF_SIZE);
  Sem_init(&client_len_mutex, 0, 1);

  listenfd = Open_listenfd(argv[1]);
  debug_print("now listening...");

  for (int i = 0; i < MAX_CONNECTIONS; i++) {
    // create threads in advance
    Pthread_create(&tid, NULL, thread, NULL);
  }

  while (1) {
    // new connection is being established
    client_len = sizeof(struct sockaddr_storage);
    connfd = Accept(listenfd, (SA *)&client_addr, &client_len);
    Getnameinfo((SA *)&client_addr, client_len, client_hostname, MAXLINE,
                client_port, MAXLINE, 0);
    printf("Connected to (%s, %s)\n", client_hostname, client_port);

    P(&client_len_mutex);
    active_client_len++;
    V(&client_len_mutex);
    sbuf_insert(&sbuf, connfd);
  }
  exit(0);
}
/* $end echoserverimain */

void *thread(void *vargp) {
  int connfd;
  Pthread_detach(pthread_self());
  while (1) {
    connfd = sbuf_remove(&sbuf);
    handle_threaded_connection(connfd);
    Close(connfd);

    P(&client_len_mutex);
    active_client_len--;
    if (!active_client_len) {
      stock_write();
    }
    V(&client_len_mutex);
  }
}