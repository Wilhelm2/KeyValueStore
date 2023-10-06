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

kv_datum* extractArrayWithoutRepetition(unsigned int size, kv_datum* array) {
    kv_datum* uniqueVector = calloc(size, sizeof(kv_datum));
    unsigned int position = 0;
    for (unsigned int i = 0; i < size; i++) {
        if (getFirstOccurrenceIndex(array[i], array, size) == i) {
            uniqueVector[position++] = array[i];
            printf("array[%d] %.*s\n", i, array[i].len, (char*)array[i].ptr);
        }
    }
    uniqueVector = realloc(uniqueVector, position * sizeof(kv_datum));
    return uniqueVector;
}

unsigned int getNbElementsArrayWithoutRepetition(unsigned int size, kv_datum* array) {
    unsigned int position = 0;
    for (unsigned int i = 0; i < size; i++) {
        if (getFirstOccurrenceIndex(array[i], array, size) == i)
            position++;
    }
    return position;
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

unsigned int getFirstOccurrenceIndex(kv_datum elt, kv_datum* array, unsigned int size) {
    for (unsigned int i = 0; i < size; i++) {
        if (array[i].len == elt.len) {
            if (memcmp(array[i].ptr, elt.ptr, elt.len) == 0)
                return i;
        }
    }
    return 0;
}

unsigned int getNbUniqueElements(kv_datum* array, unsigned int size) {
    unsigned int nbUniqueElements = 0;
    for (unsigned int i = 0; i < size; i++) {
        if (isUnique(array[i], array, size))
            nbUniqueElements++;
    }
    return nbUniqueElements;
}

void printDatabase(KV* kv) {
    kv_datum key;
    kv_datum val;
    printf("------------------------------\nDATABASE CONTENT:\n------------------------------\n");
    kv_start(kv);
    key.ptr = NULL;
    val.ptr = NULL;
    while (kv_next(kv, &key, &val) == 1) {
        printf("Key : length %u, - \n", key.len);
        write(1, key.ptr, key.len);
        printf("\nValue : length %u - \n", val.len);
        write(1, val.ptr, val.len);
        printf("\n");
        free(key.ptr);
        free(val.ptr);
        key.ptr = NULL;
        val.ptr = NULL;
    }
    printf("------------------------------\nEND OF DATABASE\n------------------------------\n");
}