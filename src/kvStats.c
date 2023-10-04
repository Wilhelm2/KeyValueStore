#include "kvStats.h"

void verifyEntriesDKV(KV* database) {
    bool error = false;
    for (unsigned int i = 0; i < getSlotsInDKV(database); i++) {
        //        printf("slot %d offset %d length %d", i, getSlotOffsetDkv(database, i), getSlotSizeDkv(database, i));
        if (i > 0) {
            //  printf("no lost space %d", (getSlotOffsetDkv(database, i) - (getSlotOffsetDkv(database, i - 1) +
            //                                                             abs(getSlotSizeDkv(database, i - 1)))) == 0);
            if (!(getSlotOffsetDkv(database, i) -
                  (getSlotOffsetDkv(database, i - 1) + abs(getSlotSizeDkv(database, i - 1)))) == 0) {
                printf("error at entry %d\n", i);
                printf("\noffsets %d %d lengths %d %d", getSlotOffsetDkv(database, i - 1),
                       getSlotOffsetDkv(database, i), getSlotSizeDkv(database, i - 1), getSlotSizeDkv(database, i));
                printf("\n");
                kv_datum key;
                fillsKey(database, getSlotOffsetDkv(database, i), &key);
                printf(" key length %d key %.*s offset %d\n", key.len, key.len, (char*)key.ptr,
                       getSlotOffsetDkv(database, i));
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