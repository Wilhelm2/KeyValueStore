#include "hashFunctions.h"

unsigned int (*getHashFunction(unsigned int index))(const kv_datum* key) {
    if (index == 1)
        return testHash;
    else if (index == 0)
        return defaultHash;
    else if (index == 2)
        return djbHash;
    else if (index == 3)
        return fnvHash;
    else
        return NULL;
}

unsigned int defaultHash(const kv_datum* key) {
    unsigned char* ptrkey = key->ptr;
    unsigned int s = 0;
    for (unsigned int i = 0; i < key->len; i++)
        s += ptrkey[i] % HASH_TABLE_SIZE;
    return (s);
}

unsigned int testHash(const kv_datum* key) {
    return key->len % 1;
}

unsigned int djbHash(const kv_datum* key) {
    unsigned int hash = 5381;
    unsigned char* ptrkey = (unsigned char*)key->ptr;
    for (unsigned int i = 0; i < key->len; i++)
        hash = 33 * hash ^ ptrkey[i];
    return hash % HASH_TABLE_SIZE;
}

unsigned int fnvHash(const kv_datum* key) {
    unsigned char* ptrkey = key->ptr;
    unsigned h = 2166136261;
    for (unsigned int i = 0; i < key->len; i++)
        h = (h * 16777619) ^ ptrkey[i];
    return h % HASH_TABLE_SIZE;
}
