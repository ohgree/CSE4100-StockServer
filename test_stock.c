#include "stock.h"

int main(int argc, const char *argv[]) {
  stock_init();

  insert(1, 10, 5000);
  insert(5, 20, 2000);
  insert(10, 5, 100);
  insert(1, 4, 5000); // should add given count to existing stock number
  insert(2, 2, 200);
  insert(2, 5, 200); // should add given count to existing stock number
  __print_db();

  insert(2, -5, 200); // attempt to remove appropriate number of stock

  insert(2, -100, 200);   // attempts to remove more than count should fail
  insert(666, -1, 31415); // attempts to remove non-existing entry should fail
  __print_db();

  stock_write(); // should write to file

  stock_init(); // stock_init() should fail if stock DB had been modified
}