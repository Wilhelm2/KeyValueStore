#include "slotAllocations.h"

int (*choix_allocation(KV* database))(KV* database, const kv_datum* key, const kv_datum* val) {
    if (database->methode_alloc == FIRST_FIT)
        return first_fit;
    if (database->methode_alloc == BEST_FIT)
        return best_fit;
    if (database->methode_alloc == WORST_FIT)
        return worst_fit;
    return first_fit;
}

// first fit : Looks for the first slot which is big enough to hold key+val
// Returns -1 when there is no available slot
int first_fit(KV* kv, const kv_datum* key, const kv_datum* val) {
    unsigned int nbSlots = *(int*)kv->dkv;
    int requiredSpace = sizeof(key->len) + key->len + sizeof(val->len) + val->len;
    int sizeOfCurrSlot;
    for (unsigned int i = 0; i < nbSlots; i++) {
        sizeOfCurrSlot = *(int*)(kv->dkv + 4 + i * 8);
        if (sizeOfCurrSlot > 0)  // empty slot
        {
            if (requiredSpace <= sizeOfCurrSlot)
                return i;
        }
    }
    return -1;  // No free slot => must create one
}

// worst fit : Returns the biggest available slot
// Returns -1 when there is no available slot
int worst_fit(KV* kv, const kv_datum* key, const kv_datum* val) {
    unsigned int nbSlots = *(int*)kv->dkv;
    int requiredSpace = sizeof(key->len) + key->len + sizeof(val->len) + val->len;
    int sizeOfCurrSlot;
    int biggestSlotIndex = -1;
    int biggestSlotSize = -1;
    // recherche le bloc le plus grand
    for (unsigned int i = 0; i < nbSlots; i++) {
        sizeOfCurrSlot = *(int*)(kv->dkv + 4 + i * 8);
        if (sizeOfCurrSlot > 0)  // emplacement vide
        {
            if (requiredSpace <= sizeOfCurrSlot && sizeOfCurrSlot > biggestSlotSize) {
                biggestSlotSize = sizeOfCurrSlot;
                biggestSlotIndex = i;
            }
        }
    }
    // returns biggest slot or -1 when there are no slot big enough
    return biggestSlotIndex;
}

// best fit : Returns the smallest slot among those big enough to store key + val
// Returns -1 when there is no available slot
int best_fit(KV* kv, const kv_datum* key, const kv_datum* val) {
    unsigned int nbSlots = *(int*)kv->dkv;
    int requiredSpace = sizeof(key->len) + key->len + sizeof(val->len) + val->len;
    int sizeOfCurrSlot;
    int bestSlotIndex = -1;
    int bestSlotSize = -1;

    for (unsigned int i = 0; i < nbSlots; i++) {
        sizeOfCurrSlot = *(int*)(kv->dkv + 4 + i * 8);  // access_lg_dkv(i, kv);
        if (requiredSpace <= sizeOfCurrSlot && sizeOfCurrSlot < bestSlotSize) {
            if (sizeOfCurrSlot == requiredSpace)
                return i;
            bestSlotIndex = i;
            bestSlotSize = sizeOfCurrSlot;
        }
    }
    return bestSlotIndex;
}
