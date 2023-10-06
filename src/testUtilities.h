#ifndef TEST_UTILITIES_H
#define TEST_UTILITIES_H

#include "kv.h"
#include "structures.h"
kv_datum* createRandomArray(unsigned int size, int maximumWordSize);
kv_datum makeRandomEntry(unsigned int lenght_max);

kv_datum* extractArrayWithoutRepetition(unsigned int size, kv_datum* array);
unsigned int getNbElementsArrayWithoutRepetition(unsigned int size, kv_datum* array);
unsigned int isUnique(kv_datum elt, kv_datum* array, unsigned int size);
unsigned int getFirstOccurrenceIndex(kv_datum elt, kv_datum* array, unsigned int size);
unsigned int getNbUniqueElements(kv_datum* array, unsigned int size);
// Prints the database. Assumes the database already to be open.
void printDatabase(KV* kv);
#endif