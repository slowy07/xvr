#include "xvr_table.h"
#include "xvr_console_colors.h"
#include "xvr_print.h"
#include "xvr_value.h"

#include <stdlib.h>
#include <string.h>

static void probeAndInsert(Xvr_Table **tableHandle, Xvr_Value key,
                           Xvr_Value value) {
  unsigned int probe = Xvr_hashValue(key) % (*tableHandle)->capacity;
  Xvr_TableEntry entry = (Xvr_TableEntry){.key = key, .value = value, .psl = 0};

  // probe
  while (true) {
    if (Xvr_checkValuesAreEqual((*tableHandle)->data[probe].key, key)) {
      (*tableHandle)->data[probe] = entry;

      (*tableHandle)->minPsl = entry.psl < (*tableHandle)->minPsl
                                   ? entry.psl
                                   : (*tableHandle)->minPsl;
      (*tableHandle)->maxPsl = entry.psl > (*tableHandle)->maxPsl
                                   ? entry.psl
                                   : (*tableHandle)->maxPsl;

      return;
    }

    // if this spot is free, insert and return
    if (XVR_VALUE_IS_NULL((*tableHandle)->data[probe].key)) {
      (*tableHandle)->data[probe] = entry;

      (*tableHandle)->count++;

      // TODO: benchmark the psl optimisation
      (*tableHandle)->minPsl = entry.psl < (*tableHandle)->minPsl
                                   ? entry.psl
                                   : (*tableHandle)->minPsl;
      (*tableHandle)->maxPsl = entry.psl > (*tableHandle)->maxPsl
                                   ? entry.psl
                                   : (*tableHandle)->maxPsl;

      return;
    }

    // if the new entry is "poorer", insert it and shift the old one
    if ((*tableHandle)->data[probe].psl < entry.psl) {
      Xvr_TableEntry tmp = (*tableHandle)->data[probe];
      (*tableHandle)->data[probe] = entry;
      entry = tmp;
    }

    // adjust and continue
    probe = (probe + 1) % (*tableHandle)->capacity;
    entry.psl++;
  }
}

// exposed functions
Xvr_Table *Xvr_private_adjustTableCapacity(Xvr_Table *oldTable,
                                           unsigned int newCapacity) {
  // allocate and zero a new table in memory
  Xvr_Table *newTable =
      malloc(newCapacity * sizeof(Xvr_TableEntry) + sizeof(Xvr_Table));

  if (newTable == NULL) {
    Xvr_error(XVR_CC_ERROR
              "ERROR: Failed to allocate a 'Xvr_Table'\n" XVR_CC_RESET);
  }

  newTable->capacity = newCapacity;
  newTable->count = 0;
  newTable->minPsl = 0;
  newTable->maxPsl = 0;

  memset(newTable + 1, 0, newTable->capacity * sizeof(Xvr_TableEntry));

  if (oldTable == NULL) { // for initial allocations
    return newTable;
  }

  for (int i = 0; i < oldTable->capacity; i++) {
    if (!XVR_VALUE_IS_NULL(oldTable->data[i].key)) {
      probeAndInsert(&newTable, oldTable->data[i].key, oldTable->data[i].value);
    }
  }

  free(oldTable);
  return newTable;
}

Xvr_Table *Xvr_allocateTable() {
  return Xvr_private_adjustTableCapacity(NULL, XVR_TABLE_INITIAL_CAPACITY);
}

void Xvr_freeTable(Xvr_Table *table) {

  if (table != NULL) {
    for (unsigned int i = 0; i < table->capacity; i++) {
      Xvr_freeValue(table->data[i].key);
      Xvr_freeValue(table->data[i].value);
    }
    free(table);
  }
}

void Xvr_insertTable(Xvr_Table **tableHandle, Xvr_Value key, Xvr_Value value) {
  if (XVR_VALUE_IS_NULL(key) || XVR_VALUE_IS_BOOLEAN(key)) {
    Xvr_error(XVR_CC_ERROR "ERROR: Bad table key\n" XVR_CC_RESET);
  }

  if ((*tableHandle)->count >
      (*tableHandle)->capacity * XVR_TABLE_EXPANSION_THRESHOLD) {
    (*tableHandle) = Xvr_private_adjustTableCapacity(
        (*tableHandle), (*tableHandle)->capacity * XVR_TABLE_EXPANSION_RATE);
  }

  probeAndInsert(tableHandle, key, value);
}

Xvr_Value Xvr_lookupTable(Xvr_Table **tableHandle, Xvr_Value key) {
  if (XVR_VALUE_IS_NULL(key) || XVR_VALUE_IS_BOOLEAN(key)) {
    Xvr_error(XVR_CC_ERROR "ERROR: Bad table key\n" XVR_CC_RESET);
  }

  // lookup
  unsigned int probe = Xvr_hashValue(key) % (*tableHandle)->capacity;

  while (true) {
    // found the entry
    if (Xvr_checkValuesAreEqual((*tableHandle)->data[probe].key, key)) {
      return (*tableHandle)->data[probe].value;
    }

    // if its an empty slot
    if (XVR_VALUE_IS_NULL((*tableHandle)->data[probe].key)) {
      return XVR_VALUE_FROM_NULL();
    }

    // adjust and continue
    probe = (probe + 1) % (*tableHandle)->capacity;
  }
}

void Xvr_removeTable(Xvr_Table **tableHandle, Xvr_Value key) {
  if (XVR_VALUE_IS_NULL(key) ||
      XVR_VALUE_IS_BOOLEAN(key)) { 
    Xvr_error(XVR_CC_ERROR "ERROR: Bad table key\n" XVR_CC_RESET);
  }

  unsigned int probe = Xvr_hashValue(key) % (*tableHandle)->capacity;
  unsigned int wipe = probe;

  while (true) {
    if (Xvr_checkValuesAreEqual((*tableHandle)->data[probe].key, key)) {
      break;
    }

    if (XVR_VALUE_IS_NULL((*tableHandle)->data[probe].key)) {
      return;
    }

    probe = (probe + 1) % (*tableHandle)->capacity;
  }

  for (unsigned int i = (*tableHandle)->minPsl; i < (*tableHandle)->maxPsl;
       i++) {
    unsigned int p = (probe + i + 0) % (*tableHandle)->capacity; // prev
    unsigned int u = (probe + i + 1) % (*tableHandle)->capacity; // current

    (*tableHandle)->data[p] = (*tableHandle)->data[u];
    (*tableHandle)->data[p].psl--;

    if (XVR_VALUE_IS_NULL((*tableHandle)->data[u].key) ||
        (*tableHandle)->data[p].psl == 0) {
      wipe = u;
      break;
    }
  }

  // finally, wipe the removed entry
  (*tableHandle)->data[wipe] = (Xvr_TableEntry){
      .key = XVR_VALUE_FROM_NULL(), .value = XVR_VALUE_FROM_NULL(), .psl = 0};
  (*tableHandle)->count--;
}
