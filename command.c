#include "command.h"

/**
 * @brief Handle connection for @p connfd
 *
 * @param connfd File descriptor for connection
 */
status_cmd handle_connection(int connfd) {
  int n, plen;
  char buf[MAXLINE];
  char response[MAXLINE];
  status_cmd status = COMMAND_ERROR;
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
 * @return Pointer to response string. NULL if client requested termination.
 */
status_cmd __handle_command(char *args[], int length, char response[]) {
  debug_print("handling command \"%s\"...", args[0]);
  status_cmd ret = COMMAND_SUCCESS;

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

status_cmd buy(int id, int n) {
  // remove item from stock db
  stock_status status;
  stock_item *item;
  status_cmd result = COMMAND_INVALID;
  if (item = search_stock(id)) {
    debug_print("item found with id=%d, count=%d, price=%d", id, item->count,
                item->price);
    if (insert(id, -n, item->price) == STOCK_SUCCESS) {
      debug_print("successfully inserted item");
      result = COMMAND_SUCCESS;
    } else {
      debug_print("failed to insert item");
    }
  } else {
    debug_print("no item found with id=%d", id);
  }

  return result;
}
status_cmd sell(int id, int n) {
  // add item to stock db
  return buy(id, -n);
}