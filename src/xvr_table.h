#ifndef XVR_TABLE_H
#define XVR_TABLE_H

#include "xvr_common.h"
#include "xvr_value.h"

typedef struct Xvr_TableEntry { // 32 | 64 BITNESS
  Xvr_Value key;                // 8  | 8
  Xvr_Value value;              // 8  | 8
  unsigned int psl;             // 4  | 4
} Xvr_TableEntry;               // 20 | 20

typedef struct Xvr_Table { // 32 | 64 BITNESS
  unsigned int capacity;   // 4  | 4
  unsigned int count;      // 4  | 4
  unsigned int minPsl;     // 4  | 4
  unsigned int maxPsl;     // 4  | 4
  Xvr_TableEntry data[];   //-  | -
} Xvr_Table;               // 16 | 16

XVR_API Xvr_Table *Xvr_allocateTable();
XVR_API void Xvr_freeTable(Xvr_Table *table);
XVR_API void Xvr_insertTable(Xvr_Table **table, Xvr_Value key, Xvr_Value value);
XVR_API Xvr_Value Xvr_lookupTable(Xvr_Table **table, Xvr_Value key);
XVR_API void Xvr_removeTable(Xvr_Table **table, Xvr_Value key);

XVR_API Xvr_Table* Xvr_private_adjustTableCapacity(Xvr_Table* oldTable, unsigned int newCapacity);

#ifndef XVR_TABLE_INITIAL_CAPACITY
#define XVR_TABLE_INITIAL_CAPACITY 8
#endif // !XVR_TABLE_INITIAL_CAPACITY

#ifndef XVR_TABLE_EXPANSION_RATE
#define XVR_TABLE_EXPANSION_RATE 2
#endif // !XVR_TABLE_EXPANSION_RATE

#ifndef XVR_TABLE_EXPANSION_THRESHOLD
#define XVR_TABLE_EXPANSION_THRESHOLD 0.8
#endif // !XVR_TABLE_EXPANSION_THRESHOLD

#endif // !XVR_TABLE_H
