#include "xvr_console_colors.h"
#include "xvr_table.h"
#include "xvr_value.h"

#include <stdio.h>

int test_table_allocation() {
  {
    Xvr_Table *table = Xvr_allocateTable();

    if (table == NULL) {
      fprintf(stderr,
              XVR_CC_ERROR "error: failed to allocate a table\n" XVR_CC_RESET);
      Xvr_freeTable(table);
      return -1;
    }
    Xvr_freeTable(table);
  }

  return 0;
}

int test_table_simple_insert_lookup_and_remove_data() {
  {
    Xvr_Table *table = Xvr_allocateTable();

    Xvr_Value key = XVR_VALUE_TO_INTEGER(1);
    Xvr_Value value = XVR_VALUE_TO_INTEGER(42);

    Xvr_insertTable(&table, key, value);
    if (table == NULL || table->capacity != 16 || table->count != 1) {
      fprintf(stderr,
              XVR_CC_ERROR "error: failed to insert into table\n" XVR_CC_ERROR);
      Xvr_freeTable(table);
      return -1;
    }

    Xvr_Value result = Xvr_lookupTable(&table, XVR_VALUE_TO_INTEGER(1));

    if (table == NULL || table->capacity != 16 || table->count != 1 ||
        XVR_VALUE_AS_INTEGER(result) != 42) {
      fprintf(stderr,
              XVR_CC_ERROR "error: failed to lookup from table\n" XVR_CC_RESET);
      Xvr_freeTable(table);
      return -1;
    }

    Xvr_removeTable(&table, XVR_VALUE_TO_INTEGER(1));

    if (table == NULL || table->capacity != 16 || table->count != 0) {
      fprintf(stderr, XVR_CC_ERROR
              "error: failed to remove from a table\n" XVR_CC_RESET);
      Xvr_freeTable(table);
      return -1;
    }
    Xvr_freeTable(table);
  }
  return 0;
}

int main() {
  int total = 0, res = 0;

  {
    res = test_table_allocation();
    if (res == 0) {
      printf(XVR_CC_NOTICE
             "test_table_allocation(): jalan lho ya rek\n" XVR_CC_RESET);
    }
    total += res;
  }

  {
    res = test_table_simple_insert_lookup_and_remove_data();
    if (res == 0) {
      printf(XVR_CC_NOTICE "nice one test table nya rek\n" XVR_CC_RESET);
    }
    total += res;
  }
  return total;
}
