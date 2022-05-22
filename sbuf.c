#include "sbuf.h"

void sbuf_init(sbuf_t *sbuf, size_t len) {
  debug_print("initialising shared buffer with len=%zu", len);
  sbuf->buf = Calloc(len, sizeof(int));
  sbuf->len = len;
  sbuf->front = sbuf->rear = 0;
  Sem_init(&sbuf->mutex, 0, 1);
  Sem_init(&sbuf->slots, 0, len);
  Sem_init(&sbuf->fds, 0, 0);
  debug_print("initialised shared buffer");
}

void sbuf_free(sbuf_t *sbuf) {
  debug_print("freeing shared buffer");
  free(sbuf->buf);
}

void sbuf_insert(sbuf_t *sbuf, int fd) {
  // mutex lock pending for slots
  P(&sbuf->slots);
  P(&sbuf->mutex);
  sbuf->buf[++sbuf->rear % sbuf->len] = fd;
  V(&sbuf->mutex);
  V(&sbuf->fds);
}
int sbuf_remove(sbuf_t *sbuf) {
  int fd;
  // mutex lock pending for fd items
  P(&sbuf->fds);
  P(&sbuf->mutex);
  fd = sbuf->buf[++sbuf->front % sbuf->len];
  V(&sbuf->mutex);
  V(&sbuf->slots);
  return fd;
}