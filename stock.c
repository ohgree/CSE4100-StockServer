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
int insert(int id, int n, int price) {
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
    };
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
    };
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

  debug_print("stock init complete. root addr.=%#010x, size=%d", stock_db.tree,
              stock_db.size);
}

/**
 * @brief Write stock database to STOCK_DB_FILENAME file.
 */
void stock_write() {
  FILE *fp;
  debug_print("writing %d entries to file", stock_db.size);
  fp = Fopen(STOCK_DB_FILENAME, "w");
  __write_item(stock_db.tree, fp);
  Fclose(fp);
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
 * @brief Recursively write @p data into file pointed by @p fp.
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
 * @brief Print all entries in database to stdout.
 */
void __print_db(void) { __write_item(stock_db.tree, stdout); }