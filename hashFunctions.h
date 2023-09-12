#ifndef HASHFUNCTIONS_H
#define HASHFUNCTIONS_H

#include "structures.h"

int (*selectHashFunction(unsigned int index))(const kv_datum* key);
int defaultHash(const kv_datum* key);
int testHash(const kv_datum* key);
int djbHash(const kv_datum* key);
int fnvHash(const kv_datum* key);

#endif