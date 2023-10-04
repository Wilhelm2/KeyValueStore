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