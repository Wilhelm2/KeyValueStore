#ifndef TEST_UTILITIES_H
#define TEST_UTILITIES_H

#include "kv.h"
#include "structures.h"

// Creates an array of size entries composed of entries of a random length between 1 and maximumWordSize.
kv_datum* createRandomArray(unsigned int size, int maximumWordSize);
// Returns a new kv_datum of a random length between 1 and length_max.
kv_datum makeRandomEntry(unsigned int lenght_max);
// Returns the array composed of elements of array without duplicates.
kv_datum* extractArrayWithoutRepetition(unsigned int size, kv_datum* array);
// Returns the number of elements in array when removing duplicates.
unsigned int getNbElementsArrayWithoutRepetition(unsigned int size, kv_datum* array);
// Returns whether elt is unique in array or not. Returns true also if elt is not contained in array.
unsigned int isUnique(kv_datum elt, kv_datum* array, unsigned int size);
// Gets the first occurrence of elt in array.
unsigned int getFirstOccurrenceIndex(kv_datum elt, kv_datum* array, unsigned int size);
// Gets the number of unique elements in array.
unsigned int getNbUniqueElements(kv_datum* array, unsigned int size);
// Prints the database. Assumes the database already to be open.
void printDatabase(KV* kv);
// Checks whether the database contains the keys in keys.
bool checkDatabaseContains(KV* database, kv_datum* keys, unsigned int size);
// Prints all keys in keys whose hash value is equal to hash.
void printAllKeysOfHash(kv_datum* keys, unsigned int size, unsigned int hash, KV* kv);
#endif