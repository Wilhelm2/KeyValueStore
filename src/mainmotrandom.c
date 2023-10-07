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
    addToDatabase(nbElementsToInsert, kv, entryArray);
    // printDatabase(kv);

    deleleteKeysInInterval(nbElementsToInsert / 2, nbElementsToInsert, kv, entryArray);
    verifyEntriesDKV(kv);

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
