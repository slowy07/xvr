#include "xvr_scope.h"
#include "xvr_bucket.h"
#include "xvr_common.h"
#include "xvr_console_colors.h"
#include "xvr_table.h"

#include "xvr_print.h"
#include "xvr_string.h"
#include "xvr_table.h"
#include "xvr_value.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// utils
static void incrementRefCount(Xvr_Scope *scope) {
  for (Xvr_Scope *iter = scope; iter; iter = iter->next) {
    iter->refCount++;
  }
}

static void decrementRefCount(Xvr_Scope *scope) {
  for (Xvr_Scope *iter = scope; iter; iter = iter->next) {
    iter->refCount--;
  }
}

static Xvr_TableEntry *lookupScope(Xvr_Scope *scope, Xvr_String *key,
                                   unsigned int hash, bool recursive) {
  // terminate
  if (scope == NULL) {
    return NULL;
  }

  // copy and modify the code from Xvr_lookupTable, so it can behave slightly
  // differently
  unsigned int probe = hash % scope->table->capacity;

  while (true) {
    // found the entry
    if (XVR_VALUE_IS_STRING(scope->table->data[probe].key) &&
        Xvr_compareStrings(XVR_VALUE_AS_STRING(scope->table->data[probe].key),
                           key) == 0) {
      return &(scope->table->data[probe]);
    }

    // if its an empty slot (didn't find it here)
    if (XVR_VALUE_IS_NULL(scope->table->data[probe].key)) {
      return recursive ? lookupScope(scope->next, key, hash, recursive) : NULL;
    }

    // adjust and continue
    probe = (probe + 1) % scope->table->capacity;
  }
}

// exposed functions
Xvr_Scope *Xvr_pushScope(Xvr_Bucket **bucketHandle, Xvr_Scope *scope) {
  Xvr_Scope *newScope = Xvr_partitionBucket(bucketHandle, sizeof(Xvr_Scope));

  newScope->next = scope;
  newScope->table = Xvr_allocateTable();
  newScope->refCount = 0;

  incrementRefCount(newScope);

  return newScope;
}

Xvr_Scope *Xvr_popScope(Xvr_Scope *scope) {
  if (scope == NULL) {
    return NULL;
  }

  decrementRefCount(scope);

  if (scope->refCount == 0) {
    Xvr_freeTable(scope->table);
    scope->table = NULL;
  }

  return scope->next;
}

Xvr_Scope *Xvr_deepCopyScope(Xvr_Bucket **bucketHandle, Xvr_Scope *scope) {
  // copy/pasted from pushScope, so I can allocate the table manually
  Xvr_Scope *newScope = Xvr_partitionBucket(bucketHandle, sizeof(Xvr_Scope));

  newScope->next = scope->next;
  newScope->table =
      Xvr_private_adjustTableCapacity(NULL, scope->table->capacity);
  newScope->refCount = 0;

  incrementRefCount(newScope);

  // forcibly copy the contents
  for (int i = 0; i < scope->table->capacity; i++) {
    if (!XVR_VALUE_IS_NULL(scope->table->data[i].key)) {
      Xvr_insertTable(&newScope->table, scope->table->data[i].key,
                      scope->table->data[i].value);
    }
  }

  return newScope;
}

void Xvr_declareScope(Xvr_Scope *scope, Xvr_String *key, Xvr_Value value) {
  if (key->type != XVR_STRING_NAME) {
    fprintf(stderr, XVR_CC_ERROR
            "ERROR: Xvr_Scope only allows name strings as keys\n" XVR_CC_RESET);
    exit(-1);
  }

  Xvr_TableEntry *entryPtr =
      lookupScope(scope, key, Xvr_hashString(key), false);

  if (entryPtr != NULL) {
    char buffer[key->length + 256];
    sprintf(buffer, "Can't redefine a variable: %s", key->as.name.data);
    Xvr_error(buffer);
    return;
  }

  Xvr_ValueType kt = Xvr_getNameStringType(key);
  if (kt != XVR_VALUE_ANY && value.type != XVR_VALUE_NULL && kt != value.type) {
    char buffer[key->length + 256];
    sprintf(buffer,
            "incorrect value type assigned to in variable declaration `%s` "
            "(expected %d, got %d)",
            key->as.name.data, (int)kt, (int)value.type);
    Xvr_error(buffer);
    return;
  }

  if (Xvr_getNameStringConstant(key) && value.type == XVR_VALUE_NULL) {
    char buffer[key->length + 256];
    sprintf(buffer, "can't declare %s as cont with value `null`",
            key->as.name.data);
    Xvr_error(buffer);
    return;
  }

  Xvr_insertTable(&scope->table, XVR_VALUE_FROM_STRING(Xvr_copyString(key)),
                  value);
}

void Xvr_assignScope(Xvr_Scope *scope, Xvr_String *key, Xvr_Value value) {
  if (key->type != XVR_STRING_NAME) {
    fprintf(stderr, XVR_CC_ERROR
            "ERROR: Xvr_Scope only allows name strings as keys\n" XVR_CC_RESET);
    exit(-1);
  }

  Xvr_TableEntry *entryPtr = lookupScope(scope, key, Xvr_hashString(key), true);

  if (entryPtr == NULL) {
    char buffer[key->length + 256];
    sprintf(buffer, "Undefined variable: %s\n", key->as.name.data);
    Xvr_error(buffer);
    return;
  }

  if (Xvr_getNameStringConstant(XVR_VALUE_AS_STRING(entryPtr->key))) {
    char buffer[key->length + 256];
    sprintf(buffer, "can't assign to const %s", key->as.name.data);
    Xvr_error(buffer);
    return;
  }
  entryPtr->value = value;
}

Xvr_Value Xvr_accessScope(Xvr_Scope *scope, Xvr_String *key) {
  if (key->type != XVR_STRING_NAME) {
    fprintf(stderr, XVR_CC_ERROR
            "ERROR: Xvr_Scope only allows name strings as keys\n" XVR_CC_RESET);
    exit(-1);
  }

  Xvr_TableEntry *entryPtr = lookupScope(scope, key, Xvr_hashString(key), true);

  if (entryPtr == NULL) {
    char buffer[key->length + 256];
    sprintf(buffer, "Undefined variable: %s", key->as.name.data);
    Xvr_error(buffer);
    return XVR_VALUE_FROM_NULL();
  }

  return entryPtr->value;
}

bool Xvr_isDeclaredScope(Xvr_Scope *scope, Xvr_String *key) {
  if (key->type != XVR_STRING_NAME) {
    fprintf(stderr, XVR_CC_ERROR
            "ERROR: Xvr_Scope only allows name strings as keys\n" XVR_CC_RESET);
    exit(-1);
  }

  Xvr_TableEntry *entryPtr = lookupScope(scope, key, Xvr_hashString(key), true);

  return entryPtr != NULL;
}
