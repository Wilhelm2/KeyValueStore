#include "kvStats.h"

void verifyEntriesDKV(KV* database) {
    bool error = false;
    for (unsigned int i = 0; i < getNbSlotsInDKV(database); i++) {
        //        printf("slot %d offset %d length %d", i, getKVOffsetDKVSlot(database, i), getDKVSlotSize(database,
        //        i));
        if (i > 0) {
            //  printf("no lost space %d", (getKVOffsetDKVSlot(database, i) - (getKVOffsetDKVSlot(database, i - 1) +
            //                                                             abs(getDKVSlotSize(database, i - 1)))) == 0);
            if (!(getKVOffsetDKVSlot(database, i) -
                  (getKVOffsetDKVSlot(database, i - 1) + abs(getDKVSlotSize(database, i - 1)))) == 0) {
                printf("error at entry %d\n", i);
                printf("\noffsets %d %d lengths %d %d", getKVOffsetDKVSlot(database, i - 1),
                       getKVOffsetDKVSlot(database, i), getDKVSlotSize(database, i - 1), getDKVSlotSize(database, i));
                printf("\n");
                kv_datum key;
                readKey(database, getKVOffsetDKVSlot(database, i), &key);
                printf(" key length %d key %.*s offset %d\n", key.len, key.len, (char*)key.ptr,
                       getKVOffsetDKVSlot(database, i));
                error = true;
            }
            // printf("\n");
        }
    }
    if (error)
        printf("ERROR IN DKV\n");
    else
        printf("NO ERROR DETECTED IN DKV\n");
}

void printStatsOnDKV(KV* database) {
    unsigned int freeSpace = 0;
    unsigned int usedSpace = 0;
    unsigned int freeSlots = 0;
    unsigned int usedSlots = 0;

    for (unsigned int i = 0; i < getNbSlotsInDKV(database); i++) {
        if (getDKVSlotSize(database, i) > 0) {  // Free slot
            freeSpace += getDKVSlotSize(database, i);
            freeSlots++;
        } else {
            usedSpace += getDKVSlotSize(database, i);
            usedSlots++;
        }
    }
    printf("Number of used slots %d, total used space %d\n", usedSlots, usedSpace);
    printf("Number of free slots %d, total free space %d\n", freeSlots, freeSpace);
}

void printSlotsDKV(KV* database) {
    for (unsigned int i = 0; i < getNbSlotsInDKV(database); i++) {
        if (getDKVSlotSize(database, i) > 0) {
            printf("Slot %d - FREE: offset %d length %d\n", i, getKVOffsetDKVSlot(database, i),
                   getDKVSlotSize(database, i));
        } else
            printf("Slot %d - TAKEN: offset %d length %d\n", i, getKVOffsetDKVSlot(database, i),
                   abs(getDKVSlotSize(database, i)));
    }
}

void printFreeSlotsDKV(KV* database) {
    for (unsigned int i = 0; i < getNbSlotsInDKV(database); i++) {
        if (getDKVSlotSize(database, i) > 0)
            printf("Slot %d - FREE: offset %d length %d\n", i, getKVOffsetDKVSlot(database, i),
                   getDKVSlotSize(database, i));
    }
}

void printTakenSlotsDKV(KV* database) {
    for (unsigned int i = 0; i < getNbSlotsInDKV(database); i++) {
        if (getDKVSlotSize(database, i) < 0)
            printf("Slot %d - TAKEN: offset %d length %d\n", i, getKVOffsetDKVSlot(database, i),
                   abs(getDKVSlotSize(database, i)));
    }
}
