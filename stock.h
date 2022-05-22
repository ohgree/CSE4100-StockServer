
#ifndef __STOCK_H__
#define __STOCK_H__

#include <semaphore.h>
#include <stdio.h>

#include "csapp.h"
#include "misc.h"

#define STOCK_DB_FILENAME "stock.txt"

enum __status {
  STOCK_FAILED = 0,
  STOCK_SUCCESS,
  STOCK_MATCH,
  STOCK_NO_MATCH,
};

struct __item {
  int id;
  int count;
  int price;
  int read_cnt;
  sem_t mutex;
  struct __item *lchild;
  struct __item *rchild;
};

struct __db {
  struct __item *tree;
  size_t size;
};

typedef enum __status stock_status;
typedef struct __item stock_item;
extern struct __db stock_db;

void stock_init(void);
void stock_write(void);
char *stock_write_to_buf(char *s);

stock_status insert(int id, int n, int price);
stock_item *search_stock(int id);

stock_item *__search(stock_item *root, int id, stock_status *status);
void __write_item(stock_item *root, FILE *fp);
void __snprint_item(stock_item *root, char *s);
void __print_db();

#endif /* __STOCK_H__ */