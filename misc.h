#ifndef __MISC_H__
#define __MISC_H__

#include <ctype.h>
#include <string.h>

#ifdef DEBUG
#define DEBUG_TEST 1
#else
#define DEBUG_TEST 0
#endif

#define DELIM_CHARS " "
#define MAX_COMMAND_ARGS 3

#define debug_print(fmt, ...)                                                  \
  do {                                                                         \
    if (DEBUG_TEST)                                                            \
      fprintf(stderr, "%s:%d:%s(): " fmt "\n", __FILE__, __LINE__, __func__,   \
              ##__VA_ARGS__);                                                  \
  } while (0)

char *ltrim(char *s);
char *rtrim(char *s);
char *trim(char *s);

char *fetch_stock_stat(void);

#endif /* __MISC_H__ */