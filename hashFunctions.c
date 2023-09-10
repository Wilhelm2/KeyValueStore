#include "hashFunctions.h"

// Return a pointer to the hash function chosen with index
int (*choix_hachage(int index))(const kv_datum* clef) {
    if (index == 1)
        return hachage_test;
    if (index == 0)
        return hachage_defaut;
    if (index == 2)
        return djb_hash;
    if (index == 3)
        return fnv_hash;
    else
        return NULL;
}

// Default hash function
int hachage_defaut(const kv_datum* clef) {
    unsigned char* ptrclef = clef->ptr;
    unsigned int i, s = 0;
    for (i = 0; i < clef->len; i++) {
        s += ptrclef[i] % 999983;
    }
    return (s);
}

// Test hash function to check whether the allocation of a new block works
int hachage_test(const kv_datum* clef) {
    return clef->len % 1;
}

// Bernstein hash
int djb_hash(const kv_datum* clef) {
    unsigned int hash = 5381, i;
    unsigned char* ptrclef = (unsigned char*)clef->ptr;

    for (i = 0; i < clef->len; i++) {
        hash = 33 * hash ^ ptrclef[i];
    }
    return hash % 999983;
}

// FNV_hash
int fnv_hash(const kv_datum* clef) {
    unsigned char* ptrclef = clef->ptr;
    unsigned h = 2166136261;
    unsigned int i;

    for (i = 0; i < clef->len; i++) {
        h = (h * 16777619) ^ ptrclef[i];
    }

    return h % 999983;
}
