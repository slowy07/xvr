#include "xvr_literal_dictionary.h"

#include <stddef.h>
#include <stdio.h>

#include "xvr_console_colors.h"
#include "xvr_literal.h"
#include "xvr_memory.h"

static void setEntryValues(Xvr_private_entry* entry, Xvr_Literal key,
                           Xvr_Literal value) {
    Xvr_freeLiteral(entry->key);
    entry->key = Xvr_copyLiteral(key);

    Xvr_freeLiteral(entry->value);
    entry->value = Xvr_copyLiteral(value);
}

static Xvr_private_entry* getEntryArray(Xvr_private_entry* array, int capacity,
                                        Xvr_Literal key, unsigned int hash,
                                        bool mustExist) {
    unsigned int index = hash % capacity;
    unsigned int start = index;

    index = (index + 1) % capacity;

    while (index != start) {
        Xvr_private_entry* entry = &array[index];

        if (XVR_IS_NULL(entry->key)) {
            if (XVR_IS_NULL(entry->value) && !mustExist) {
                return entry;
            }
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
    Xvr_private_entry* newEntries = XVR_ALLOCATE(Xvr_private_entry, capacity);

    for (int i = 0; i < capacity; i++) {
        newEntries[i].key = XVR_TO_NULL_LITERAL;
        newEntries[i].value = XVR_TO_NULL_LITERAL;
    }

    for (int i = 0; i < oldCapacity; i++) {
        if (XVR_IS_NULL((*dictionaryHandle)[i].key)) {
            continue;
        }

        Xvr_private_entry* entry =
            getEntryArray(newEntries, capacity, XVR_TO_NULL_LITERAL,
                          Xvr_hashLiteral((*dictionaryHandle)[i].key), false);

        entry->key = (*dictionaryHandle)[i].key;
        entry->value = (*dictionaryHandle)[i].value;
    }

    XVR_FREE_ARRAY(Xvr_private_entry, *dictionaryHandle, oldCapacity);
    *dictionaryHandle = newEntries;
}

static bool setEntryArray(Xvr_private_entry** dictionaryHandle,
                          int* capacityPtr, int contains, Xvr_Literal key,
                          Xvr_Literal value, int hash) {
    if (contains + 1 > *capacityPtr * XVR_DICTIONARY_MAX_LOAD) {
        int oldCapacity = *capacityPtr;
        *capacityPtr = XVR_GROW_CAPACITY(*capacityPtr);
        adjustEntryCapacity(dictionaryHandle, oldCapacity, *capacityPtr);
    }

    Xvr_private_entry* entry =
        getEntryArray(*dictionaryHandle, *capacityPtr, key, hash, false);

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

void Xvr_initLiteralDictionary(Xvr_LiteralDictionary* dictionary) {
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
                "dictionary can't have null keys (set)\n" XVR_CC_RESET);
        return;
    }

    if (XVR_IS_FUNCTION(key) || XVR_IS_FUNCTION_NATIVE(key)) {
        fprintf(stderr, XVR_CC_ERROR
                "dictionary can't have opaque keys (set)\n" XVR_CC_RESET);
        return;
    }

    if (XVR_IS_OPAQUE(key)) {
        fprintf(stderr, XVR_CC_ERROR
                "dictionary can't have opaque keys (set)\n" XVR_CC_RESET);
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
                "dictionary can't have null keys (get)\n" XVR_CC_RESET);
        return XVR_TO_NULL_LITERAL;
    }

    if (XVR_IS_FUNCTION(key) || XVR_IS_FUNCTION_NATIVE(key)) {
        fprintf(stderr, XVR_CC_ERROR
                "dictionary can't have function keys (get)\n" XVR_CC_RESET);
        return XVR_TO_NULL_LITERAL;
    }

    if (XVR_IS_OPAQUE(key)) {
        fprintf(stderr, XVR_CC_ERROR
                "dictionary can't have opaque (get)\n" XVR_CC_RESET);
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
                "dictionary can't have null keys (remove)\n" XVR_CC_RESET);
        return;
    }

    if (XVR_IS_OPAQUE(key)) {
        fprintf(stderr, XVR_CC_ERROR
                "dictionary can't have opaque keys (remove)\n" XVR_CC_RESET);
        return;
    }

    Xvr_private_entry* entry =
        getEntryArray(dictionary->entries, dictionary->capacity, key,
                      Xvr_hashLiteral(key), true);

    if (entry != NULL) {
        freeEntry(entry);
        entry->value = XVR_TO_BOOLEAN_LITERAL(true);
        dictionary->count--;
    }
}

bool Xvr_existsLiteralDictionary(Xvr_LiteralDictionary* dictionary,
                                 Xvr_Literal key) {
    Xvr_private_entry* entry =
        getEntryArray(dictionary->entries, dictionary->capacity, key,
                      Xvr_hashLiteral(key), false);
    return !(XVR_IS_NULL(entry->key) && XVR_IS_NULL(entry->value));
}
