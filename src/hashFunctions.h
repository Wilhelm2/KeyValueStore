#ifndef HASHFUNCTIONS_H
#define HASHFUNCTIONS_H

#include "structures.h"
#define HASH_TABLE_SIZE 20000

// Returns a hash function following index
unsigned int (*getHashFunction(unsigned int index))(const kv_datum* key);
// Default hash function
unsigned int defaultHash(const kv_datum* key);
// Test hash function that returns the same hash for any key
unsigned int testHash(const kv_datum* key);
// Bernstein hash
unsigned int djbHash(const kv_datum* key);
// FNV_hash
unsigned int fnvHash(const kv_datum* key);

#endif