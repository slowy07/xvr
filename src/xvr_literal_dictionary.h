/**
MIT License

Copyright (c) 2025 arfy slowy

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

/**
 * @brief hash map - core associative container for XVR
 *
 * Xvr_LiteralDictionary implements an open-addressing hash table with linear
 * probing optimized for
 *  - fast lookups of identifier, string keys, and small integers
 *  - value semantices (deep copies on insert)
 *  - automatic resizing to maintain performance
 *  - gracefull handling of hash collisiion and tombstone
 *
 * memory Layout:
 *     +---------------------+------------+-------+----------+
 *     | Xvr_private_entry*  | int cap    | count | contains |
 *     +---------------------+------------+-------+----------+
 *     `entries` points to array of `Xvr_private_entry`, each holding:
 *         - key: `Xvr_Literal` (copied)
 *         - value: `Xvr_Literal` (copied)
 *
 * performance:
 * - O(1) average insert / lookup / delete (amortized)
 * - O(n) worst-case (high collision)
 * - resize triggered `count / capacity > XVR_DICTIONARY_MAX_LOAD`
 */

#ifndef XVR_LITERAL_DICTIONARY_H
#define XVR_LITERAL_DICTIONARY_H

#include "xvr_common.h"
#include "xvr_literal.h"

/**
 * @def XVR_DICTIONARY_MAX_LOAD
 * @brief max load factor before resize (0.75 = 75% full)
 *
 * trade-off
 * - lower (0.5): fewer collision, more memory
 * - higher (0.9): denser, but probe chains grow -> slower groups
 *
 * 0.75 max load as like (java hashmap, and python dictionary with version 3.6
 * -> higher)
 */
#define XVR_DICTIONARY_MAX_LOAD 0.75

/**
 * @struct Xvr_private_entry
 * @brief internal key-value pair storage
 */
typedef struct Xvr_private_entry {
    Xvr_Literal key;    // owned copy of key literal
    Xvr_Literal value;  // owned copy of value literal
} Xvr_private_entry;

/**
 * @struct Xvr_LiteralDictionary
 * @brief hash table with open addressing
 *
 * fields:
 *  - entries: heap-allocated array of `Xvr_private_entry`
 *  - capacity: number of slots (alwats power-of-two for fast module)
 *  - contains: number of logical key-value pairs
 *
 * invariants
 * - capacity is 0 or power-of-two (enable hash & (capacity - 1) indexing)
 * - if capacity == 0 then entries == NULL, count == 0
 *- count <= capacity * XVR_DICTIONARY_MAX_LOAD
 */
typedef struct Xvr_LiteralDictionary {
    Xvr_private_entry* entries;  // array of entries
    int capacity;                // allocated slots
    int count;                   // activate entries
    int contains;                // logical nairs
} Xvr_LiteralDictionary;

/**
 * @brief intialize an empty dictionary
 *
 * @param[out] dictionary dictionary to initialize (must not be NULL)
 *
 * sets
 * - entries = NULL
 *   - capacity = 0
 *   - contains = 0
 *
 *   @note idempotent and safe on zeroed memory
 *       first insert will allocate initial table (example, 8 slot)
 */
XVR_API void Xvr_initLiteralDictionary(Xvr_LiteralDictionary* dictionary);

/**
 * @brief free all resource held by dictionary
 *
 * behaviour:
 *   - for each occupied entry: call Xvr_freeLiteral() on `key` and `value`
 *   - free `entries` buffer
 *   - reset all fields to zero
 *
 * @param[in, out] dictionary dictionary to destroy (must not be NULL)
 * @note Nil-safe: on-op if already free
 */
XVR_API void Xvr_freeLiteralDictionary(Xvr_LiteralDictionary* dictionary);

/**
 * @brief insert or update a key-value pair
 *
 * @param[in, out] dictionary dictionary to modify
 * @param[in] key key literal (copied; must be hashable)
 * @param[in] value value literal (copied)
 *
 * hashable rule (enforcing in impl):
 * NULL, BOOLEAN, INTEGER, STRING, IDENTIFIER
 *
 * @note O(1) amortized, may trigger `realloc` -> `entries` pointer may change
 */
XVR_API void Xvr_setLiteralDictionary(Xvr_LiteralDictionary* dictionary,
                                      Xvr_Literal key, Xvr_Literal value);

/**
 * @brief retrieves value for key
 *
 * @param[in] dictionary dictionary to query
 * @param[key] key key to lookup
 * @return value if found, or `XVR_TO_NULL_LITERAL` if:
 *  - key not present
 *  - key is unhasable
 *  - dictionary is empty / corrupt
 *
 */
XVR_API Xvr_Literal Xvr_getLiteralDictionary(Xvr_LiteralDictionary* dictionary,
                                             Xvr_Literal key);

/**
 * @brief remove key and its value from the dictionary
 *
 * @param[in, out] dictionary dictionary to modify
 * @param[in] key key to remove
 *
 * @note use backward shift deletion (not tombstone) to avoiding probe chain
 * fragmentation O(1) average, O(n) are the worst-case (long probe chain)
 */
XVR_API void Xvr_removeLiteralDictionary(Xvr_LiteralDictionary* dictionary,
                                         Xvr_Literal key);

/**
 * @brief check if key exists in dictionary
 *
 * @param[in] dictionary dictionary to query
 * @param[in] key key to check
 * @return `true` if key is present and hashable, false otherwise
 *
 * @note faster than `get` + null check (avoiding copying value)
 * prefered for existence test
 */
XVR_API bool Xvr_existsLiteralDictionary(Xvr_LiteralDictionary* dictionary,
                                         Xvr_Literal key);

#endif  // !XVR_LITERAL_DICTIONARY_H
