#include "command.h"

static sem_t mutex;
static int byte_len;

static void __init_threaded_connection(void) {
  byte_len = 0;
  Sem_init(&mutex, 0, 1);
}

/**
 * @brief Handle connection for @p connfd
 *
 * @param connfd File descriptor for connection
 * @return Command execution's resulting status code
 */
cmd_status handle_connection(int connfd) {
  int n, plen;
  char buf[MAXLINE];
  char response[MAXLINE];
  cmd_status status = COMMAND_ERROR;
  rio_t rio;

  memset(response, 0, sizeof(response));

  Rio_readinitb(&rio, connfd);
  if (!(n = Rio_readlineb(&rio, buf, MAXLINE))) {
    // handle client termination via ctrl-c
    debug_print("client terminated the connection");
    return COMMAND_EXIT;
  } else {
    char *pbuf[MAX_COMMAND_ARGS];
    debug_print("server received %d bytes", n);

    plen = __parse(rtrim(buf), pbuf);
    for (int i = 0; i < plen; i++) {
      debug_print("pbuf[%d] = \"%s\"", i, pbuf[i]);
    }

    status = __handle_command(pbuf, plen, response);
    // write size must be equal to client Rio_readnb() read size
    Rio_writen(connfd, response, MAXLINE);
    debug_print("response to client: \"%s\"", response);
  }
  debug_print("handler returned with status %d", status);

  return status;
}

void handle_threaded_connection(int connfd) {
  int n, plen;
  char buf[MAXLINE];
  char response[MAXLINE];
  cmd_status status = COMMAND_ERROR;
  rio_t rio;

  static pthread_once_t once = PTHREAD_ONCE_INIT;

  Pthread_once(&once, __init_threaded_connection);

  Rio_readinitb(&rio, connfd);
  while ((n = Rio_readlineb(&rio, buf, MAXLINE))) {
    char *pbuf[MAX_COMMAND_ARGS];

    memset(response, 0, sizeof(response));
    debug_print("server received %d bytes", n);

    plen = __parse(rtrim(buf), pbuf);
    for (int i = 0; i < plen; i++) {
      debug_print("pbuf[%d] = \"%s\"", i, pbuf[i]);
    }

    status = __handle_command(pbuf, plen, response);

    P(&mutex);
    byte_len += n;
    debug_print(
        "server received %d (%d total) bytes on thread 0x%02x with fd=%d", n,
        byte_len, (unsigned long)pthread_self(), connfd);
    V(&mutex);

    // write size must be equal to client Rio_readnb() read size
    Rio_writen(connfd, response, MAXLINE);
    debug_print("response to client: \"%s\"", response);
    debug_print("handler returned with status %d", status);
    if (status == COMMAND_EXIT) {
      break;
    }
  }
}

/**
 * @brief Parse given command string into blocks.
 * @warning Execution is destructive for argument @p cmd.
 *
 * @param cmd Null-terminated command string
 * @param buf Buffer to store pointers for each argument
 * @return Number of parsed command arguments
 */
size_t __parse(char *cmd, char **buf) {
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
 *
 * @param args Argument list for command string.
 * @param length Length for @p args.
 * @param response Reference to variable for storing response string.
 * @return Resulting status code
 */
cmd_status __handle_command(char *args[], int length, char response[]) {
  debug_print("handling command \"%s\"...", args[0]);
  cmd_status ret = COMMAND_SUCCESS;

  if (length == 1) {
    if (!strcmp(args[0], "exit")) {
      // client requested termination
      strcpy(response, "\n");
      return COMMAND_EXIT;
    } else if (!strcmp(args[0], "show")) {
      // current stock status
      stock_write_to_buf(response);
    } else {
      debug_print("invalid command \"%s\"", args[0]);
      strcpy(response, "invalid command\n");
      return COMMAND_INVALID;
    }
  } else if (length == 3) {
    int id = atoi(args[1]);
    int count = atoi(args[2]);
    debug_print("handling 3-arg command: %s %d %d", args[0], id, count);

    if (!strcmp(args[0], "buy")) {
      // remove item from stock
      ret = buy(id, count);
    } else if (!strcmp(args[0], "sell")) {
      // add item to stock
      // TODO: handle trying to sell non-existing stock id
      ret = sell(id, count);
    }

    if (ret == COMMAND_SUCCESS) {
      sprintf(response, "[%s] success\n", args[0]);
    } else {
      strcpy(response, "Not enough left stocks\n");
    }
  } else {
    debug_print("invalid command \"%s\"", args[0]);
    strcpy(response, "invalid command\n");
    return COMMAND_INVALID;
  }
  return ret;
}

/**
 * @brief Remove item from stack db.
 *
 * @param id id of the stock item to buy.
 * @param n number of stock items to buy.
 * @return cmd_status typed enum:
 *  - #COMMAND_SUCCESS if db was successfully modified
 *  - #COMMAND_INVALID if no such item with given id was found, or when item
 * count was not enough
 */
cmd_status buy(int id, int n) {
  // remove item from stock db
  int is_number_valid;
  stock_item *item;
  cmd_status result = COMMAND_INVALID;
  if ((item = search_stock(id))) {
    debug_print("item found with id=%d, count=%d, price=%d", id, item->count,
                item->price);

    P(&item->r_mutex);
    if (++item->read_cnt == 1) {
      P(&item->w_mutex); // first in - lock write
    }
    V(&item->r_mutex);

    is_number_valid = item->count >= n;

    P(&item->r_mutex);
    if (--item->read_cnt == 0) {
      V(&item->w_mutex); // last out - unlock write
    }
    V(&item->r_mutex);

    if (is_number_valid) {
      P(&item->w_mutex);
      item->count -= n;
      V(&item->w_mutex);
      result = COMMAND_SUCCESS;
      debug_print("successfully inserted item");
    } else {
      debug_print("failed to insert item");
    }

    debug_print("no item found with id=%d", id);
  }

  return result;
}

/**
 * @brief Add item to stack db.
 *
 * @param id id of the stock item to sell.
 * @param n number of stock items to sell.
 * @return cmd_status typed enum:
 *  - #COMMAND_SUCCESS if db was successfully updated
 *  - #COMMAND_INVALID if no such item with given id was found
 */
cmd_status sell(int id, int n) {
  // add item to stock db
  stock_item *item;
  cmd_status result = COMMAND_INVALID;
  if ((item = search_stock(id))) {
    debug_print("item found with id=%d, count=%d, price=%d", id, item->count,
                item->price);

    P(&item->w_mutex); //  lock write
    item->count += n;
    V(&item->w_mutex); // unlock write

    debug_print("successfully inserted item");
    result = COMMAND_SUCCESS;
  } else {
    debug_print("failed to insert item");
  }
  return result;
}