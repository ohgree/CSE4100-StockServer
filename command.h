#ifndef __COMMAND_H__
#define __COMMAND_H__

#include "csapp.h"
#include "misc.h"
#include "stock.h"

typedef enum {
  COMMAND_ERROR = 0,
  COMMAND_SUCCESS,
  COMMAND_EXIT,
  COMMAND_INVALID,
} status_cmd;

status_cmd handle_connection(int connfd);
status_cmd buy(int id, int n);
status_cmd sell(int id, int n);

size_t __parse(char *cmd, char **buf);
status_cmd __handle_command(char *args[], int length, char response[]);

#endif /* __COMMAND_H__ */