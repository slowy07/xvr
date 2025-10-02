#include "xvr_table.h"
#include "xvr_console_colors.h"
#include "xvr_print.h"
#include "xvr_value.h"

#include <stdlib.h>
#include <string.h>

#define MIN_CAPACITY 16

static void probeAndInsert(Xvr_Table **table, Xvr_Value key, Xvr_Value value) {
  unsigned int probe = Xvr_hashValue(key) % (*table)->capacity;
  Xvr_TableEntry entry = (Xvr_TableEntry){.key = key, .value = value, .psl = 0};

  while (true) {
    if (XVR_VALUES_ARE_EQUAL((*table)->data[probe].key, key)) {
      (*table)->data[probe] = entry;

      (*table)->minPsl =
          entry.psl < (*table)->minPsl ? entry.psl : (*table)->minPsl;
      (*table)->maxPsl =
          entry.psl > (*table)->maxPsl ? entry.psl : (*table)->maxPsl;
      return;
    }

    if (XVR_VALUE_IS_NULL((*table)->data[probe].key)) {
      (*table)->data[probe] = entry;
      (*table)->count++;

      (*table)->minPsl =
          entry.psl < (*table)->minPsl ? entry.psl : (*table)->minPsl;
      (*table)->maxPsl =
          entry.psl > (*table)->maxPsl ? entry.psl : (*table)->maxPsl;
      return;
    }

    if ((*table)->data[probe].psl < entry.psl) {
      Xvr_TableEntry tmp = (*table)->data[probe];
      (*table)->data[probe] = entry;
      entry = tmp;
    }
    probe = (probe + 1) % (*table)->capacity;
    entry.psl++;
  }
}

static Xvr_Table *adjustTableCapacity(Xvr_Table *oldTable,
                                      unsigned int newCapacity) {
  // allocate and zero a new table in memory
  Xvr_Table *newTable =
      malloc(newCapacity * sizeof(Xvr_TableEntry) + sizeof(Xvr_Table));

  if (newTable == NULL) {
    Xvr_error(XVR_CC_ERROR
              "Error: failed to allocate `Xvr_Table`\n" XVR_CC_RESET);
  }

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
    if (!XVR_VALUE_IS_NULL(oldTable->data[i].key)) {
      probeAndInsert(&newTable, oldTable->data[i].key, oldTable->data[i].value);
    }
  }

  // clean up and return
  free(oldTable);
  return newTable;
}

// exposed functions
Xvr_Table *Xvr_allocateTable() {
  return adjustTableCapacity(NULL, MIN_CAPACITY);
}

void Xvr_freeTable(Xvr_Table *table) { free(table); }

void Xvr_insertTable(Xvr_Table **table, Xvr_Value key, Xvr_Value value) {
  if (XVR_VALUE_IS_NULL(key) || XVR_VALUE_IS_BOOLEAN(key)) {
    Xvr_error(XVR_CC_ERROR "Error: bad table key\n" XVR_CC_RESET);
  }

  // expand the capacity
  if ((*table)->count > (*table)->capacity * 0.8) {
    (*table) = adjustTableCapacity(*table, (*table)->capacity * 2);
  }

  probeAndInsert(table, key, value);
}

Xvr_Value Xvr_lookupTable(Xvr_Table **table, Xvr_Value key) {
  if (XVR_VALUE_IS_NULL(key) || XVR_VALUE_IS_BOOLEAN(key)) {
    Xvr_error(XVR_CC_ERROR "Error: bad table key\n" XVR_CC_RESET);
  }

  // lookup
  unsigned int probe = Xvr_hashValue(key) % (*table)->capacity;

  while (true) {
    // found the entry
    if (XVR_VALUES_ARE_EQUAL((*table)->data[probe].key, key)) {
      return (*table)->data[probe].value;
    }

    // if the psl is too big, or empty slot
    if (XVR_VALUE_IS_NULL((*table)->data[probe].key)) {
      return XVR_VALUE_FROM_NULL();
    }

    probe = (probe + 1) % (*table)->capacity;
  }
}

void Xvr_removeTable(Xvr_Table **table, Xvr_Value key) {
  if (XVR_VALUE_IS_NULL(key) || XVR_VALUE_IS_BOOLEAN(key)) {
    Xvr_error(XVR_CC_ERROR "Error: bad table key\n" XVR_CC_RESET);
  }

  // lookup
  unsigned int probe = Xvr_hashValue(key) % (*table)->capacity;
  unsigned int wipe = probe; // wiped at the end

  while (true) {
    // found the entry
    if (XVR_VALUES_ARE_EQUAL((*table)->data[probe].key, key)) {
      break;
    }

    // if the psl is too big, or empty slot
    if (XVR_VALUE_IS_NULL((*table)->data[probe].key)) {
      return;
    }

    // adjust and continue
    probe = (probe + 1) % (*table)->capacity;
  }

  // shift down the later entries (past the probing point)
  for (unsigned int i = (*table)->minPsl; i < (*table)->maxPsl; i++) {
    unsigned int p = (probe + i + 0) % (*table)->capacity; // prev
    unsigned int u = (probe + i + 1) % (*table)->capacity; // current

    // if the psl is too big, or an empty slot, stop
    if (XVR_VALUE_IS_NULL((*table)->data[u].key) ||
        (*table)->data[p].psl == 0) {
      wipe = u;
      break;
    }
  }

  // finally, wipe the removed entry
  (*table)->data[wipe] = (Xvr_TableEntry){
      .key = XVR_VALUE_FROM_NULL(), .value = XVR_VALUE_FROM_NULL(), .psl = 0};
  (*table)->count--;
}
