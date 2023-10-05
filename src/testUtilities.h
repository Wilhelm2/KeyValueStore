#ifndef TEST_UTILITIES_H
#define TEST_UTILITIES_H

#include "structures.h"
kv_datum* createRandomArray(unsigned int size, int maximumWordSize);
kv_datum makeRandomEntry(unsigned int lenght_max);

kv_datum* extractUniqueEntriesArray(unsigned int size, kv_datum* array);
unsigned int isUnique(kv_datum elt, kv_datum* array, unsigned int size);
unsigned int getNbUniqueArrayElements(kv_datum* array, unsigned int size);
#endif