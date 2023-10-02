#include "slotAllocations.h"

int (*getAllocationMethod(KV* database))(KV* database, const kv_datum* key, const kv_datum* val) {
    if (database->allocationMethod == FIRST_FIT)
        return first_fit;
    if (database->allocationMethod == BEST_FIT)
        return best_fit;
    if (database->allocationMethod == WORST_FIT)
        return worst_fit;
    return first_fit;
}

int first_fit(KV* kv, const kv_datum* key, const kv_datum* val) {
    unsigned int nbSlots = getSlotsInDKV(kv);
    int requiredSpace = sizeof(key->len) + key->len + sizeof(val->len) + val->len;
    int sizeOfCurrSlot;
    for (unsigned int i = 0; i < nbSlots; i++) {
        sizeOfCurrSlot = getSlotSizeDkv(kv, i);
        if (sizeOfCurrSlot > 0) {
            if (requiredSpace <= sizeOfCurrSlot)
                return i;
        }
    }
    return -1;
}

int worst_fit(KV* kv, const kv_datum* key, const kv_datum* val) {
    unsigned int nbSlots = getSlotsInDKV(kv);
    int requiredSpace = sizeof(key->len) + key->len + sizeof(val->len) + val->len;
    int sizeOfCurrSlot;
    int biggestSlotIndex = -1;
    int biggestSlotSize = -1;
    for (unsigned int i = 0; i < nbSlots; i++) {
        sizeOfCurrSlot = getSlotSizeDkv(kv, i);
        if (sizeOfCurrSlot > 0) {
            if (requiredSpace <= sizeOfCurrSlot && sizeOfCurrSlot > biggestSlotSize) {
                biggestSlotSize = sizeOfCurrSlot;
                biggestSlotIndex = i;
            }
        }
    }
    return biggestSlotIndex;
}

int best_fit(KV* kv, const kv_datum* key, const kv_datum* val) {
    unsigned int nbSlots = getSlotsInDKV(kv);
    int requiredSpace = sizeof(key->len) + key->len + sizeof(val->len) + val->len;
    int sizeOfCurrSlot;
    int bestSlotIndex = -1;
    int bestSlotSize = -1;

    for (unsigned int i = 0; i < nbSlots; i++) {
        sizeOfCurrSlot = getSlotSizeDkv(kv, i);
        if (requiredSpace <= sizeOfCurrSlot && sizeOfCurrSlot < bestSlotSize) {
            if (sizeOfCurrSlot == requiredSpace)
                return i;
            bestSlotIndex = i;
            bestSlotSize = sizeOfCurrSlot;
        }
    }
    return bestSlotIndex;
}
