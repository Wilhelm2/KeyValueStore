#include "testUtilities.h"

kv_datum* createRandomArray(unsigned int size, int maximumWordSize) {
    kv_datum* tableau = calloc(size, sizeof(kv_datum));
    for (unsigned int i = 0; i < size; i++)
        tableau[i] = makeRandomEntry(maximumWordSize);
    return tableau;
}

kv_datum makeRandomEntry(unsigned int length_max) {
    kv_datum kv;
    kv.len = rand() % length_max + 1;
    kv.ptr = malloc(kv.len + 1);  // +1 for '\0'
    for (unsigned int i = 0; i < kv.len; i++) {
        ((char*)kv.ptr)[i] = 'a' + (rand() % 27);
    }
    ((char*)kv.ptr)[kv.len] = '\0';

    return kv;
}

kv_datum* extractUniqueEntriesArray(unsigned int size, kv_datum* array) {
    kv_datum* uniqueVector = calloc(size, sizeof(kv_datum));
    unsigned int position = 0;
    for (unsigned int i = 0; i < size; i++) {
        if (isUnique(array[i], array, size)) {
            uniqueVector[position++] = array[i];
        }
    }
    uniqueVector = realloc(uniqueVector, position * sizeof(kv_datum));
    return uniqueVector;
}

unsigned int isUnique(kv_datum elt, kv_datum* array, unsigned int size) {
    bool hasSeen = false;
    for (unsigned int i = 0; i < size; i++) {
        if (array[i].len == elt.len) {
            if (memcmp(array[i].ptr, elt.ptr, elt.len) == 0) {
                if (hasSeen == true)
                    return false;
                hasSeen = true;
            }
        }
    }
    return true;
}

unsigned int getNbUniqueArrayElements(kv_datum* array, unsigned int size) {
    unsigned int nbUniqueElements = 0;
    for (unsigned int i = 0; i < size; i++) {
        if (isUnique(array[i], array, size))
            nbUniqueElements++;
    }
    return nbUniqueElements;
}