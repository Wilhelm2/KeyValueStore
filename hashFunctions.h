#ifndef HASHFUNCTIONS_H
#define HASHFUNCTIONS_H

#include "structures.h"
#define HASH_TABLE_SIZE 20000

unsigned int (*selectHashFunction(unsigned int index))(const kv_datum* key);
unsigned int defaultHash(const kv_datum* key);
unsigned int testHash(const kv_datum* key);
unsigned int djbHash(const kv_datum* key);
unsigned int fnvHash(const kv_datum* key);

#endif