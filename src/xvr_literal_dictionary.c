#include "xvr_literal_dictionary.h"

#include <stddef.h>
#include <stdio.h>

#include "xvr_console_colors.h"
#include "xvr_literal.h"
#include "xvr_memory.h"

static void setEntryValues(Xvr_private_entry* entry, Xvr_Literal key,
                           Xvr_Literal value) {
    // much simpler now
    Xvr_freeLiteral(entry->key);
    entry->key = Xvr_copyLiteral(key);

    Xvr_freeLiteral(entry->value);
    entry->value = Xvr_copyLiteral(value);
}

static Xvr_private_entry* getEntryArray(Xvr_private_entry* array, int capacity,
                                        Xvr_Literal key, unsigned int hash,
                                        bool mustExist) {
    // find "key", starting at index
    unsigned int index = hash % capacity;
    unsigned int start = index;

    // increment once, so it can't equal start
    index = (index + 1) % capacity;

    // literal probing and collision checking
    while (index != start) {  // WARNING: this is the only function allowed to
                              // retrieve an entry from the array
        Xvr_private_entry* entry = &array[index];

        if (XVR_IS_NULL(entry->key)) {  // if key is empty, it's either empty or
                                        // tombstone
            if (XVR_IS_NULL(entry->value) && !mustExist) {
                // found a truly empty bucket
                return entry;
            }
            // else it's a tombstone - ignore
        } else {
            if (Xvr_literalsAreEqual(key, entry->key)) {
                return entry;
            }
        }

        index = (index + 1) % capacity;
    }

    return NULL;
}

static void adjustEntryCapacity(Xvr_private_entry** dictionaryHandle,
                                int oldCapacity, int capacity) {
    // new entry space
    Xvr_private_entry* newEntries = XVR_ALLOCATE(Xvr_private_entry, capacity);

    for (int i = 0; i < capacity; i++) {
        newEntries[i].key = XVR_TO_NULL_LITERAL;
        newEntries[i].value = XVR_TO_NULL_LITERAL;
    }

    // move the old array into the new one
    for (int i = 0; i < oldCapacity; i++) {
        if (XVR_IS_NULL((*dictionaryHandle)[i].key)) {
            continue;
        }

        // place the key and value in the new array (reusing string memory)
        Xvr_private_entry* entry =
            getEntryArray(newEntries, capacity, XVR_TO_NULL_LITERAL,
                          Xvr_hashLiteral((*dictionaryHandle)[i].key), false);

        entry->key = (*dictionaryHandle)[i].key;
        entry->value = (*dictionaryHandle)[i].value;
    }

    // clear the old array
    XVR_FREE_ARRAY(Xvr_private_entry, *dictionaryHandle, oldCapacity);

    *dictionaryHandle = newEntries;
}

static bool setEntryArray(Xvr_private_entry** dictionaryHandle,
                          int* capacityPtr, int contains, Xvr_Literal key,
                          Xvr_Literal value, int hash) {
    // expand array if needed
    if (contains + 1 > *capacityPtr * XVR_DICTIONARY_MAX_LOAD) {
        int oldCapacity = *capacityPtr;
        *capacityPtr = XVR_GROW_CAPACITY(*capacityPtr);
        adjustEntryCapacity(
            dictionaryHandle, oldCapacity,
            *capacityPtr);  // custom rather than automatic reallocation
    }

    Xvr_private_entry* entry =
        getEntryArray(*dictionaryHandle, *capacityPtr, key, hash, false);

    // true = contains increase
    if (XVR_IS_NULL(entry->key)) {
        setEntryValues(entry, key, value);
        return true;
    } else {
        setEntryValues(entry, key, value);
        return false;
    }

    return false;
}

static void freeEntry(Xvr_private_entry* entry) {
    Xvr_freeLiteral(entry->key);
    Xvr_freeLiteral(entry->value);
    entry->key = XVR_TO_NULL_LITERAL;
    entry->value = XVR_TO_NULL_LITERAL;
}

static void freeEntryArray(Xvr_private_entry* array, int capacity) {
    if (array == NULL) {
        return;
    }

    for (int i = 0; i < capacity; i++) {
        if (!XVR_IS_NULL(array[i].key)) {
            freeEntry(&array[i]);
        }
    }

    XVR_FREE_ARRAY(Xvr_private_entry, array, capacity);
}

// exposed functions
void Xvr_initLiteralDictionary(Xvr_LiteralDictionary* dictionary) {
    // HACK: because modulo by 0 is undefined, set the capacity to a non-zero
    // value (and allocate the arrays)
    dictionary->entries = NULL;
    dictionary->capacity = XVR_GROW_CAPACITY(0);
    dictionary->contains = 0;
    dictionary->count = 0;
    adjustEntryCapacity(&dictionary->entries, 0, dictionary->capacity);
}

void Xvr_freeLiteralDictionary(Xvr_LiteralDictionary* dictionary) {
    freeEntryArray(dictionary->entries, dictionary->capacity);
    dictionary->capacity = 0;
    dictionary->contains = 0;
}

void Xvr_setLiteralDictionary(Xvr_LiteralDictionary* dictionary,
                              Xvr_Literal key, Xvr_Literal value) {
    if (XVR_IS_NULL(key)) {
        fprintf(stderr, XVR_CC_ERROR
                "Dictionaries can't have null keys (set)\n" XVR_CC_RESET);
        return;
    }

    if (XVR_IS_FUNCTION(key) || XVR_IS_FUNCTION_NATIVE(key) ||
        XVR_IS_FUNCTION_HOOK(key)) {
        fprintf(stderr, XVR_CC_ERROR
                "Dictionaries can't have function keys (set)\n" XVR_CC_RESET);
        return;
    }

    if (XVR_IS_OPAQUE(key)) {
        fprintf(stderr, XVR_CC_ERROR
                "Dictionaries can't have opaque keys (set)\n" XVR_CC_RESET);
        return;
    }

    const int increment =
        setEntryArray(&dictionary->entries, &dictionary->capacity,
                      dictionary->contains, key, value, Xvr_hashLiteral(key));

    if (increment) {
        dictionary->contains++;
        dictionary->count++;
    }
}

Xvr_Literal Xvr_getLiteralDictionary(Xvr_LiteralDictionary* dictionary,
                                     Xvr_Literal key) {
    if (XVR_IS_NULL(key)) {
        fprintf(stderr, XVR_CC_ERROR
                "Dictionaries can't have null keys (get)\n" XVR_CC_RESET);
        return XVR_TO_NULL_LITERAL;
    }

    if (XVR_IS_FUNCTION(key) || XVR_IS_FUNCTION_NATIVE(key) ||
        XVR_IS_FUNCTION_HOOK(key)) {
        fprintf(stderr, XVR_CC_ERROR
                "Dictionaries can't have function keys (get)\n" XVR_CC_RESET);
        return XVR_TO_NULL_LITERAL;
    }

    if (XVR_IS_OPAQUE(key)) {
        fprintf(stderr, XVR_CC_ERROR
                "Dictionaries can't have opaque keys (get)\n" XVR_CC_RESET);
        return XVR_TO_NULL_LITERAL;
    }

    Xvr_private_entry* entry =
        getEntryArray(dictionary->entries, dictionary->capacity, key,
                      Xvr_hashLiteral(key), true);

    if (entry != NULL) {
        return Xvr_copyLiteral(entry->value);
    } else {
        return XVR_TO_NULL_LITERAL;
    }
}

void Xvr_removeLiteralDictionary(Xvr_LiteralDictionary* dictionary,
                                 Xvr_Literal key) {
    if (XVR_IS_NULL(key)) {
        fprintf(stderr, XVR_CC_ERROR
                "Dictionaries can't have null keys (remove)\n" XVR_CC_RESET);
        return;
    }

    if (XVR_IS_FUNCTION(key) || XVR_IS_FUNCTION_NATIVE(key) ||
        XVR_IS_FUNCTION_HOOK(key)) {
        fprintf(
            stderr, XVR_CC_ERROR
            "Dictionaries can't have function keys (remove)\n" XVR_CC_RESET);
        return;
    }

    if (XVR_IS_OPAQUE(key)) {
        fprintf(stderr, XVR_CC_ERROR
                "Dictionaries can't have opaque keys (remove)\n" XVR_CC_RESET);
        return;
    }

    Xvr_private_entry* entry =
        getEntryArray(dictionary->entries, dictionary->capacity, key,
                      Xvr_hashLiteral(key), true);

    if (entry != NULL) {
        freeEntry(entry);
        entry->value = XVR_TO_BOOLEAN_LITERAL(true);  // tombstone
        dictionary->count--;
    }
}

bool Xvr_existsLiteralDictionary(Xvr_LiteralDictionary* dictionary,
                                 Xvr_Literal key) {
    // null & not tombstoned
    Xvr_private_entry* entry =
        getEntryArray(dictionary->entries, dictionary->capacity, key,
                      Xvr_hashLiteral(key), false);
    return !(XVR_IS_NULL(entry->key) && XVR_IS_NULL(entry->value));
}
