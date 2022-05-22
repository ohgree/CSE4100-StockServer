#include "stock.h"

/* global variable for stock data */
struct __db stock_db = {
    .tree = NULL,
    .size = 0,
};

/**
 * @brief Insert @p n stock entry with given @p id and @p price.
 *
 * @param id Unique ID for the stock
 * @param n Number of stocks to insert. May be negative in order to decrease
 * stock count
 * @param price Price of the stock
 * @return STOCK_SUCCESS by default, STOCK_FAILED when there are not
 * enough number of stocks to remove
 */
stock_status insert(int id, int n, int price) {
  stock_status status;
  debug_print("inserting id=%d, n=%d, price=%d", id, n, price);
  stock_item *item = __search(stock_db.tree, id, &status);

  if (n < 0 && status != STOCK_MATCH) {
    debug_print("tried to remove count from non-existing entry. failing...");
    return STOCK_FAILED;
  }

  if (stock_db.size == 0) {
    // empty db. initialize new item as root
    stock_item *new = (stock_item *)Malloc(sizeof(stock_item));
    *new = (stock_item){
        .id = id,
        .count = n,
        .price = price,
        .read_cnt = 0,
    };
    Sem_init(&new->r_mutex, 0, 1);
    Sem_init(&new->w_mutex, 0, 1);

    stock_db.tree = new;
    stock_db.size++;
    return STOCK_SUCCESS;
  }

  if (status == STOCK_MATCH) {
    // pre-existing item was found
    debug_print("attempting to update id=%d's count from %d to %d", item->id,
                item->count, item->count + n);
    if (item->count + n < 0) {
      debug_print("not enough items left. insert failed.");
      return STOCK_FAILED;
    }
    item->count += n;
    return STOCK_SUCCESS;
  } else {
    // next insert position was returned
    stock_item *new = (stock_item *)Malloc(sizeof(stock_item));
    *new = (stock_item){
        .id = id,
        .count = n,
        .price = price,
        .read_cnt = 0,
    };
    Sem_init(&new->r_mutex, 0, 1);
    Sem_init(&new->w_mutex, 0, 1);

    if (item->id < id) {
      item->rchild = new;
    } else {
      item->lchild = new;
    }
    stock_db.size++;
    return STOCK_SUCCESS;
  }
}

/**
 * @brief Initialise stock database.
 * @note Fails when stock database had been modified in any way.
 */
void stock_init(void) {
  int id, count, price;

  if (stock_db.tree || stock_db.size) {
    unix_error("stock_init() should not be called after any modification");
  }

  debug_print("initialising with data in %s", STOCK_DB_FILENAME);
  FILE *fp = Fopen(STOCK_DB_FILENAME, "r");

  while (fscanf(fp, "%d %d %d", &id, &count, &price) != EOF) {
    debug_print("found entry id=%d count=%d price=%d", id, count, price);
    insert(id, count, price);
  }
  Fclose(fp);

  debug_print("stock init complete. root addr.=%#010x, size=%zu", stock_db.tree,
              stock_db.size);
}

/**
 * @brief Write stock database to STOCK_DB_FILENAME file.
 */
void stock_write() {
  FILE *fp;
  debug_print("writing %zu entries to file", stock_db.size);
  fp = Fopen(STOCK_DB_FILENAME, "w");
  __write_item(stock_db.tree, fp);
  Fclose(fp);
}

/**
 * @brief Print stock database to @p s buffer.
 *
 * @param s Reference of buffer to print to.
 * @return Pointer to written buffer.
 */
char *stock_write_to_buf(char *s) {
  memset(s, 0, strlen(s));
  debug_print("writing %zu entries to buffer", stock_db.size);
  __snprint_item(stock_db.tree, s);
  return s;
}

/**
 * @brief Search for stock item in db with matching @p id
 *
 * @param id ID of stock item to search for.
 * @return Pointer to stock item with matching @p id. NULL if no such item were
 * found.
 */
stock_item *search_stock(int id) {
  stock_status status;
  stock_item *item;
  debug_print("searching for stock with id=%d", id);
  item = __search(stock_db.tree, id, &status);
  if (status != STOCK_MATCH) {
    return NULL;
  }
  return item;
}

/**
 * @brief Search for position in @p root suitable for @p id to be in.
 *
 * @param root Root of the tree
 * @param id ID to search for
 * @param[out] status Reference to variable. Set to STOCK_MATCH when
 * matching entry is found, else STOCK_NO_MATCH.
 * @return If matching entry was found, that entry's pointer is returned. If
 * not, the parent entry's pointer is returned. Returns NULL if @p root is NULL.
 */
stock_item *__search(stock_item *root, int id, stock_status *status) {
  *status = STOCK_NO_MATCH;
  if (!root) {
    debug_print("root is empty");
    return NULL;
  }

  debug_print("traversing root->id=%d. lchild=%#010x, rchild=%#010x", root->id,
              root->lchild, root->rchild);
  if (root->id == id) {
    debug_print("item matches id");
    *status = STOCK_MATCH;
    return root;
  }

  stock_item *next_root = NULL;

  if (root->id < id) {
    if (root->rchild) {
      next_root = root->rchild;
    } else {
      debug_print("can be inserted as rchild of id=%d", root->id);
      return root;
    }
  } else {
    if (root->lchild) {
      next_root = root->lchild;
    } else {
      debug_print("can be inserted as lchild of id=%d", root->id);
      return root;
    }
  }
  return __search(next_root, id, status); // tail call optimisation
}

/**
 * @brief Recursively write @p data into @p fp.
 *
 * @param root Root of the stock database to write
 * @param fp File pointer to write to
 */
void __write_item(stock_item *root, FILE *fp) {
  if (!root) {
    return;
  }
  __write_item(root->lchild, fp);
  fprintf(fp, "%d %d %d\n", root->id, root->count, root->price);
  __write_item(root->rchild, fp);
}

/**
 * @brief Print entries in database @p root to buffer @p s.
 *
 * @param root Root of the stock database.
 * @param s buffer to write to.
 */
void __snprint_item(stock_item *root, char *s) {
  char buf[MAXLINE];
  if (!root) {
    return;
  }
  debug_print("on node id=%d", root->id);

  __snprint_item(root->lchild, s);

  P(&root->r_mutex); // lock read
  if (++root->read_cnt == 1) {
    P(&root->w_mutex); // lock write
  }
  V(&root->r_mutex);

  snprintf(buf, sizeof(buf), "%d %d %d\n", root->id, root->count, root->price);

  P(&root->r_mutex); // lock read
  if (--root->read_cnt == 0) {
    V(&root->w_mutex); // unlock write
  }
  V(&root->r_mutex);

  strcat(s, buf);
  __snprint_item(root->rchild, s);
}

/**
 * @brief Print all entries in database to stdout.
 */
void __print_db(void) { __write_item(stock_db.tree, stdout); }