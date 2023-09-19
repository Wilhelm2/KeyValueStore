#include "hashFunctions.h"

// Return a pointer to the hash function chosen with index
unsigned int (*selectHashFunction(unsigned int index))(const kv_datum* key) {
    if (index == 1)
        return testHash;
    if (index == 0)
        return defaultHash;
    if (index == 2)
        return djbHash;
    if (index == 3)
        return fnvHash;
    else
        return NULL;
}

// Default hash function
unsigned int defaultHash(const kv_datum* key) {
    unsigned char* ptrkey = key->ptr;
    unsigned int s = 0;
    for (unsigned int i = 0; i < key->len; i++) {
        s += ptrkey[i] % HASH_TABLE_SIZE;
    }
    return (s);
}

// Test hash function to check whether the allocation of a new block works
unsigned int testHash(const kv_datum* key) {
    return key->len % 1;
}

// Bernstein hash
unsigned int djbHash(const kv_datum* key) {
    unsigned int hash = 5381;
    unsigned char* ptrkey = (unsigned char*)key->ptr;
    for (unsigned int i = 0; i < key->len; i++) {
        hash = 33 * hash ^ ptrkey[i];
    }
    return hash % HASH_TABLE_SIZE;
}

// FNV_hash
unsigned int fnvHash(const kv_datum* key) {
    unsigned char* ptrkey = key->ptr;
    unsigned h = 2166136261;
    for (unsigned int i = 0; i < key->len; i++) {
        h = (h * 16777619) ^ ptrkey[i];
    }
    return h % HASH_TABLE_SIZE;
}
