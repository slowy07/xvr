#include "xvr_table.h"
#include "xvr_console_colors.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MIN_CAPACITY 16

static Xvr_Table *adjustTableCapacity(Xvr_Table *oldTable,
                                      unsigned int newCapacity) {
  // allocate and zero a new table in memory
  Xvr_Table *newTable =
      malloc(newCapacity * sizeof(Xvr_TableEntry) + sizeof(Xvr_Table));

  newTable->capacity = newCapacity;
  newTable->count = 0;
  newTable->minPsl = 0;
  newTable->maxPsl = 0;

  // unlike other structures, the empty space in a table needs to be null
  memset(newTable + 1, 0, newTable->capacity * sizeof(Xvr_TableEntry));

  if (oldTable == NULL) { // for initial allocations
    return newTable;
  }

  // for each entry in the old table, copy it into the new table
  for (int i = 0; i < oldTable->capacity; i++) {
    Xvr_insertTable(&newTable, oldTable->data[i].key, oldTable->data[i].value);
  }

  // clean up and return
  free(oldTable);
  return newTable;
}

// exposed functions
Xvr_Table *Xvr_allocateTable() {
  return adjustTableCapacity(NULL, MIN_CAPACITY);
}

void Xvr_freeTable(Xvr_Table *table) {
  free(table);
}

void Xvr_insertTable(Xvr_Table **table, Xvr_Value key, Xvr_Value value) {
  if (XVR_VALUE_IS_NULL(key) ||
      XVR_VALUE_IS_BOOLEAN(key)) {
    fprintf(stderr, XVR_CC_ERROR "ERROR: Bad table key\n" XVR_CC_RESET);
    exit(-1);
  }

  // expand the capacity
  if ((*table)->capacity < (*table)->count * (1 / 0.75f)) {
    (*table) = adjustTableCapacity(*table, (*table)->capacity * 2);
  }

  // insert
  unsigned int probe = Xvr_hashValue(key) % (*table)->capacity;
  Xvr_TableEntry entry = (Xvr_TableEntry){.key = key, .value = value, .psl = 0};

  while (true) {
    // if this spot is free, insert and return
    if (XVR_VALUE_IS_NULL((*table)->data[probe].key)) {
      (*table)->data[probe] = entry;

      (*table)->count++;

      (*table)->minPsl =
          entry.psl < (*table)->minPsl ? entry.psl : (*table)->minPsl;
      (*table)->maxPsl =
          entry.psl > (*table)->maxPsl ? entry.psl : (*table)->maxPsl;

      return;
    }

    // if the new entry is "poorer", insert it and shift the old one
    if ((*table)->data[probe].psl < entry.psl) {
      Xvr_TableEntry tmp = (*table)->data[probe];
      (*table)->data[probe] = entry;
      entry = tmp;
    }

    // adjust and continue
    probe = (probe + 1) % (*table)->capacity;
    entry.psl++;
  }
}

Xvr_Value Xvr_lookupTableValue(Xvr_Table **table, Xvr_Value key) {
  if (XVR_VALUE_IS_NULL(key) ||
      XVR_VALUE_IS_BOOLEAN(key)) {
    fprintf(stderr, XVR_CC_ERROR "ERROR: Bad table key\n" XVR_CC_RESET);
    exit(-1);
  }

  // lookup
  unsigned int probe = Xvr_hashValue(key) % (*table)->capacity;
  unsigned int counter = 0;

  while (true) {
    // found the entry
    if (XVR_VALUE_IS_EQUAL((*table)->data[probe].key, key)) {
      return (*table)->data[probe].value;
    }

    // if the psl is too big, or empty slot
    if ((*table)->data[probe].psl > counter ||
        XVR_VALUE_IS_NULL((*table)->data[probe].key)) {
      return XVR_VALUE_TO_NULL();
    }

    // adjust and continue
    probe = (probe + 1) % (*table)->capacity;
    counter++;
  }
}

void Xvr_removeTableEntry(Xvr_Table **table, Xvr_Value key) {
  if (XVR_VALUE_IS_NULL(key) ||
      XVR_VALUE_IS_BOOLEAN(key)) {
    fprintf(stderr, XVR_CC_ERROR "ERROR: Bad table key\n" XVR_CC_RESET);
    exit(-1);
  }

  // lookup
  unsigned int probe = Xvr_hashValue(key) % (*table)->capacity;
  unsigned int counter = 0;
  unsigned int wipe = probe; // wiped at the end

  while (true) {
    // found the entry
    if (XVR_VALUE_IS_EQUAL((*table)->data[probe].key, key)) {
      break;
    }

    // if the psl is too big, or empty slot
    if ((*table)->data[probe].psl > counter ||
        XVR_VALUE_IS_NULL((*table)->data[probe].key)) {
      return;
    }

    // adjust and continue
    probe = (probe + 1) % (*table)->capacity;
    counter++;
  }

  // shift down the later entries (past the probing point)
  for (unsigned int i = (*table)->minPsl; i < (*table)->maxPsl; i++) {
    unsigned int p = (probe + i + 0) % (*table)->capacity; // prev
    unsigned int u = (probe + i + 1) % (*table)->capacity; // current

    // if the psl is too big, or an empty slot, stop
    if ((*table)->data[u].psl > (counter + i) ||
        XVR_VALUE_IS_NULL((*table)->data[u].key)) {
      break;
    }

    (*table)->data[p] = (*table)->data[u];
    (*table)->data[p].psl--;
    wipe = wipe % (*table)->capacity;
  }

  // finally, wipe the removed entry
  (*table)->data[wipe] = (Xvr_TableEntry){
      .key = XVR_VALUE_TO_NULL(), .value = XVR_VALUE_TO_NULL(), .psl = 0};
}
