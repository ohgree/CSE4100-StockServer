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
} cmd_status;

cmd_status handle_connection(int connfd);
void handle_threaded_connection(int connfd);
cmd_status buy(int id, int n);
cmd_status sell(int id, int n);

size_t __parse(char *cmd, char **buf);
cmd_status __handle_command(char *args[], int length, char response[]);

#endif /* __COMMAND_H__ */