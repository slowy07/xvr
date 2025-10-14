#include <stdio.h>

#include "xvr_console_colors.h"
#include "xvr_table.h"
#include "xvr_value.h"

int test_table_allocation() {
    {
        Xvr_Table* table = Xvr_allocateTable();

        if (table == NULL) {
            fprintf(stderr, XVR_CC_ERROR
                    "error: failed to allocate a table\n" XVR_CC_RESET);
            Xvr_freeTable(table);
            return -1;
        }
        Xvr_freeTable(table);
    }

    return 0;
}

int test_table_simple_insert_lookup_and_remove_data() {
    {
        Xvr_Table* table = Xvr_allocateTable();

        Xvr_Value key = XVR_VALUE_FROM_INTEGER(1);
        Xvr_Value value = XVR_VALUE_FROM_INTEGER(42);

        Xvr_insertTable(&table, key, value);
        if (table == NULL || table->capacity != 8 || table->count != 1) {
            fprintf(stderr, XVR_CC_ERROR
                    "error: failed to insert into table\n" XVR_CC_ERROR);
            Xvr_freeTable(table);
            return -1;
        }

        Xvr_Value result = Xvr_lookupTable(&table, XVR_VALUE_FROM_INTEGER(1));

        if (table == NULL || table->capacity != 8 || table->count != 1 ||
            XVR_VALUE_AS_INTEGER(result) != 42) {
            fprintf(stderr, XVR_CC_ERROR
                    "error: failed to lookup from table\n" XVR_CC_RESET);
            Xvr_freeTable(table);
            return -1;
        }

        Xvr_removeTable(&table, XVR_VALUE_FROM_INTEGER(1));

        if (table == NULL || table->capacity != 8 || table->count != 0) {
            fprintf(stderr, XVR_CC_ERROR
                    "error: failed to remove from a table\n" XVR_CC_RESET);
            Xvr_freeTable(table);
            return -1;
        }
        Xvr_freeTable(table);
    }
    return 0;
}

int test_table_expansion() {
    {
        Xvr_Table* table = Xvr_allocateTable();
        int top = 300;

        for (int i = 0; i < 400; i++) {
            Xvr_insertTable(&table, XVR_VALUE_FROM_INTEGER(i),
                            XVR_VALUE_FROM_INTEGER(top - i));
        }

        Xvr_Value result = Xvr_lookupTable(&table, XVR_VALUE_FROM_INTEGER(265));

        if (table == NULL || table->capacity != 512 || table->count != 400 ||
            XVR_VALUE_IS_INTEGER(result) != true ||
            XVR_VALUE_AS_INTEGER(result) != 35) {
            fprintf(stderr, XVR_CC_ERROR
                    "error: table expansion under stress test "
                    "failed\n" XVR_CC_RESET);
            Xvr_freeTable(table);
            return -1;
        }
        Xvr_freeTable(table);
    }

    {
        Xvr_Table* table = Xvr_allocateTable();

        for (int i = 0; i < 20; i++) {
            Xvr_insertTable(&table, XVR_VALUE_FROM_INTEGER(i),
                            XVR_VALUE_FROM_INTEGER(100 - i));
        }

        Xvr_Value result = Xvr_lookupTable(&table, XVR_VALUE_FROM_INTEGER(15));

        if (table == NULL || table->capacity != 32 || table->count != 20 ||
            XVR_VALUE_IS_INTEGER(result) != true ||
            XVR_VALUE_AS_INTEGER(result) != 85) {
            fprintf(stderr, XVR_CC_ERROR
                    "error: bad result table lookup after expansion "
                    "with collision and modulo wrap\n" XVR_CC_RESET);
            Xvr_freeTable(table);
            return -1;
        }
        Xvr_freeTable(table);
    }

    return 0;
}

int main() {
    printf(XVR_CC_WARN "TESTING: XVR TABLE\n" XVR_CC_RESET);
    int total = 0, res = 0;

    {
        res = test_table_allocation();
        if (res == 0) {
            printf(XVR_CC_NOTICE
                   "TABLE ALLOCATION: PASSED jalan lho ya rek\n" XVR_CC_RESET);
        }
        total += res;
    }

    {
        res = test_table_simple_insert_lookup_and_remove_data();
        if (res == 0) {
            printf(XVR_CC_NOTICE
                   "TABLE SIMPLE INSERT LOOKUP AND REMOVE DATA: PASSED "
                   "nice one test table nya rek\n" XVR_CC_RESET);
        }
        total += res;
    }

    {
        res = test_table_expansion();
        if (res == 0) {
            printf(XVR_CC_NOTICE
                   "TABLE EXPANSION: PASSED aman rek\n" XVR_CC_RESET);
        }
        total += res;
    }

    return total;
}
