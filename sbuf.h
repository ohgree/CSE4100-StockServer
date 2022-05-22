#ifndef __SBUF_H__
#define __SBUF_H__

#include "csapp.h"
#include "misc.h"

#define SBUF_SIZE 20

struct __sbuf_t {
  int *buf;
  size_t len;
  int front;
  int rear;
  sem_t mutex;
  sem_t slots;
  sem_t fds;
};

typedef struct __sbuf_t sbuf_t;

void sbuf_init(sbuf_t *sbuf, size_t len);
void sbuf_free(sbuf_t *sbuf);

void sbuf_insert(sbuf_t *sbuf, int fd);
int sbuf_remove(sbuf_t *sbuf);

#endif /* __SBUF_H__ */