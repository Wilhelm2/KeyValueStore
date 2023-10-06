// WILHELM Daniel YAMAN Kendal
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/times.h>
#include <time.h>
#include <unistd.h>
#include "kv.h"
#include "kvStats.h"
#include "testUtilities.h"
int deleleteKeysInterval(unsigned int i, unsigned int j, KV* kv, kv_datum* tableau);
void fillsDatabase(unsigned int n, KV* kv, kv_datum* tableau);
void freeArray(kv_datum* array, unsigned int size);
bool checkDatabaseContains(KV* database, kv_datum* keys, unsigned int size);
void printAllKeysWithSameHash(kv_datum* keys, unsigned int size, unsigned int hash, KV* kv);

// Inserts elements in table with key going from 0 to i-1 and values going from 1 to i
void fillsDatabase(unsigned int n, KV* kv, kv_datum* tableau) {
    for (unsigned int i = 0; i < n; i++) {
        printf("\t\t writes key %d length %d key %.*s nbelements %d\n", i, tableau[i].len, tableau[i].len,
               (char*)tableau[i].ptr, getNbSlotsInDKV(kv));

        //        printf("inserts element %d\n", i);
        kv_put(kv, &tableau[i], &tableau[(i + 1) % n]);
    }
}

// Checks whether the database contains the keys of the array
bool checkDatabaseContains(KV* database, kv_datum* keys, unsigned int size) {
    kv_datum val;
    for (unsigned int i = 0; i < size; i++) {
        printf("looks up key length %d key %.*s\n", keys[i].len, keys[i].len, (char*)keys[i].ptr);
        if (kv_get(database, &keys[i], &val) == 0) {
            printf("key %d not found\n", i);
            printAllKeysWithSameHash(keys, size, database->hashFunction(&keys[i]), database);
            return false;
        }
    }
    return true;
}

void printAllKeysWithSameHash(kv_datum* keys, unsigned int size, unsigned int hash, KV* kv) {
    for (unsigned int j = 0; j < size; j++) {
        if (hash == kv->hashFunction(&keys[j]))
            printf("key %d length %d key %.*s\n", j, keys[j].len, keys[j].len, (char*)keys[j].ptr);
    }
}

// Deletes all keys contained in the array between index i to j. Returns -1 if one of the delete fails.
int deleleteKeysInterval(unsigned int i, unsigned int j, KV* kv, kv_datum* array) {
    int test = 0;
    for (unsigned int k = i; k < j; k++) {
        if (kv_del(kv, &array[k]) == -1)
            test = -1;
    }
    return test;
}

void freeArray(kv_datum* array, unsigned int size) {
    for (unsigned int i = 0; i < size; i++) {
        free(array[i].ptr);
    }
    free(array);
}

int main(int argc, char* argv[]) {
    if (argc < 6) {
        printf("usage %s : <nom de base> <nbElementsToInsert> <taille_max_mot> <methode d'allocation> <mode> <hidx>\n",
               argv[0]);
        exit(1);
    }
    unsigned int nbElementsToInsert = atoi(argv[2]);
    unsigned int maximumWordSize = atoi(argv[3]);
    alloc_t allocationMethod = atoi(argv[4]);
    unsigned int hidx = atoi(argv[6]);

    // Opens the database
    KV* kv = kv_open(argv[1], argv[5], hidx, allocationMethod);
    if (kv == NULL) {
        printf("Error while opening the database\n");
        exit(1);
    }

    // Builds the array to fill the database
    kv_datum* entryArray = createRandomArray(nbElementsToInsert, maximumWordSize);
    fillsDatabase(nbElementsToInsert, kv, entryArray);
    // printDatabase(kv);

    // printf("Now deletes elements from database\n");
    deleleteKeysInterval(nbElementsToInsert / 2, nbElementsToInsert, kv, entryArray);
    verifyEntriesDKV(kv);
    kv_datum* uniqueVector = extractArrayWithoutRepetition(nbElementsToInsert, entryArray);
    printf(
        "all keys are contained in tab %d\n",
        checkDatabaseContains(kv, uniqueVector, getNbElementsArrayWithoutRepetition(nbElementsToInsert, entryArray)));

    //    affiche_base(kv);
    // printStatsOnDKV(kv);
    // printSlotsDKV(kv);
    // printFreeSlotsDKV(kv);
    // printTakenSlotsDKV(kv);
    // printElementsPerBlock(kv);
    // averageKeyLength(kv);
    // averageValueLength(kv);

    if (kv_close(kv) == -1)
        printf("Error while closing the database\n");
    freeArray(entryArray, nbElementsToInsert);
}
