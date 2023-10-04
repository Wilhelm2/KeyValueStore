#include "functions_dkv.h"

int allocatesDkvSlot(KV* database, const kv_datum* key, const kv_datum* val) {
    int dkvSlot;
    if ((dkvSlot = getAllocationMethod(database)(database, key, val)) == -1) {  // no free slot, so creates one
        createNewSlotEndDKV(database, key, val);
        dkvSlot = getSlotsInDKV(database) - 1;
    }
    SetSlotDKVAsOccupied(database, dkvSlot);
    trySplitDKVSlot(database, dkvSlot, key->len + sizeof(key->len) + val->len + sizeof(val->len));
    return dkvSlot;
}

// Creates new slot at the end of DKV
void createNewSlotEndDKV(KV* kv, const kv_datum* key, const kv_datum* val) {
    len_t offsetNewSlot;
    printf("allocates new slot at end of dkv\n");
    if (getSlotsInDKV(kv) == kv->dkvh.maxElementsInDKV)
        increaseSizeDkv(kv);

    len_t lengthNewSlot = sizeof(len_t) + key->len + sizeof(len_t) + val->len;
    memcpy(kv->dkvh.dkv + getOffsetDkv(getSlotsInDKV(kv)), &lengthNewSlot, sizeof(len_t));
    if (getSlotsInDKV(kv) == 0)
        offsetNewSlot = HEADER_SIZE_KV;
    else
        offsetNewSlot = getSlotOffsetDkv(kv, getSlotsInDKV(kv) - 1) + abs(getSlotSizeDkv(kv, getSlotsInDKV(kv) - 1));
    memcpy(kv->dkvh.dkv + getOffsetDkv(getSlotsInDKV(kv)) + sizeof(len_t), &offsetNewSlot, sizeof(len_t));
    printf("writes length %d of new slot at offset %d\n", lengthNewSlot, getOffsetDkv(getSlotsInDKV(kv)));
    printf("writes offset %d of new slot at offset %d\n", offsetNewSlot, getSlotOffsetDkv(kv, getSlotsInDKV(kv)));
    printf("writes offset %d of new slot at dkvoffset %d\n", offsetNewSlot, getOffsetDkv(getSlotsInDKV(kv)));
    if (getSlotsInDKV(kv) > 0) {
        printf("previous slot offset %d length %d\n", getSlotOffsetDkv(kv, getSlotsInDKV(kv) - 1),
               abs(getSlotSizeDkv(kv, getSlotsInDKV(kv) - 1)));
    }
    setSlotsInDKV(kv, getSlotsInDKV(kv) + 1);
    printf("incr nbslots to %d in createnewslotendkv\n", getSlotsInDKV(kv));
}

// Increases the size of dkv
void increaseSizeDkv(KV* database) {
    database->dkvh.maxElementsInDKV += 1000;
    database->dkvh.dkv = realloc(database->dkvh.dkv, sizeOfDKVMax(database));
}

// Sets DKV slot as occupied by multiplying its length by -1
void SetSlotDKVAsOccupied(KV* kv, int dkvSlot) {
    int length = getSlotSizeDkv(kv, dkvSlot) * (-1);
    memcpy(kv->dkvh.dkv + getOffsetDkv(dkvSlot), &length, sizeof(int));
}

// Splits the DKV slot in two if there is enough space to contain another slot
void trySplitDKVSlot(KV* kv, unsigned int slotToSplit, unsigned int slotToSplitRequiredLength) {
    len_t remainingBytes = (-1) * getSlotSizeDkv(kv, slotToSplit) - slotToSplitRequiredLength;
    if (remainingBytes > (2 * sizeof(unsigned int) + 2)) {
        printf("splits the slot %d\n", slotToSplit);
        // Size for another slot of at least one character for key and value
        insertNewSlotDKV(kv, slotToSplit);
        setSlotSizeDkv(kv, slotToSplit, slotToSplitRequiredLength * (-1));  //*-1 to indicate that slot is taken
        setSlotSizeDkv(kv, slotToSplit + 1, remainingBytes);
        setSlotOffsetDkv(kv, slotToSplit + 1, getSlotOffsetDkv(kv, slotToSplit) + slotToSplitRequiredLength);
    }
}

// Shifts slots one to the right
void insertNewSlotDKV(KV* database, int firstSlot) {
    if (getSlotsInDKV(database) == database->dkvh.maxElementsInDKV)  // adds space to dkv
        increaseSizeDkv(database);

    memcpy(database->dkvh.dkv + getOffsetDkv(firstSlot + 1), database->dkvh.dkv + getOffsetDkv(firstSlot),
           sizeOfDKVFilled(database) - getOffsetDkv(firstSlot));
    setSlotsInDKV(database, getSlotsInDKV(database) + 1);
    printf("incr nbslots to %d in insertnewslotdkv\n", getSlotsInDKV(database));
}

void freeDKVSlot(len_t offsetKV, KV* kv) {
    for (unsigned int i = 0; i < getSlotsInDKV(kv); i++) {
        if (getSlotOffsetDkv(kv, i) == offsetKV) {
            int length = (-1) * getSlotSizeDkv(kv, i);  // multiplies by -1 to tag the slot as free
            memcpy(kv->dkvh.dkv + getOffsetDkv(i), &length, sizeof(int));
            trySlotMerge(kv, i);               // Tries to merge the free slot with right and left slots
            if (i == (getSlotsInDKV(kv) - 1))  // the newly freed slot is the last one of DKV
                setSlotsInDKV(kv, getSlotsInDKV(kv) - 1);
            return;
        }
    }
}

void trySlotMerge(KV* database, unsigned int centralSlot) {
    if (centralSlot > 0) {                                    // has slot to the left
        if (getSlotSizeDkv(database, centralSlot - 1) > 0) {  // left slot is free -> merge
            mergeSlots(centralSlot - 1, centralSlot, database);
            centralSlot--;  // because has just removed the left slot
        }
    }
    if (centralSlot < getSlotsInDKV(database) - 1) {        // has slot to the right
        if (getSlotSizeDkv(database, centralSlot + 1) > 0)  // right slot is free -> merge
            mergeSlots(centralSlot, centralSlot + 1, database);
    }
}

void mergeSlots(unsigned int left, unsigned int right, KV* database) {
    printf("merge slots %d and %d", left, right);
    printf("offset %d and %d\n", getSlotOffsetDkv(database, left), getSlotOffsetDkv(database, right));
    printf("length %d and %d\n", getSlotSizeDkv(database, left), getSlotSizeDkv(database, right));
    len_t newSlotSize = getSlotSizeDkv(database, left) + getSlotSizeDkv(database, right);
    len_t newSlotOffset = getSlotOffsetDkv(database, left);
    memcpy(database->dkvh.dkv + getOffsetDkv(left), database->dkvh.dkv + getOffsetDkv(right),
           sizeOfDKVFilled(database) - getOffsetDkv(right));  // Moves dkv one to the left to remove right slot
    memcpy(database->dkvh.dkv + getOffsetDkv(left), &newSlotSize, sizeof(len_t));
    memcpy(database->dkvh.dkv + getOffsetDkv(left) + sizeof(int), &newSlotOffset, sizeof(len_t));
    setSlotsInDKV(database, getSlotsInDKV(database) - 1);
}

len_t getKVSize(KV* database) {
    len_t maxOffset = 0;
    len_t maxLength = 0;
    unsigned int nbElementsInDKV = getSlotsInDKV(database);
    for (unsigned int i = 0; i < nbElementsInDKV; i++) {
        if (maxOffset <= getSlotOffsetDkv(database, i)) {
            maxOffset = getSlotOffsetDkv(database, i);
            maxLength = abs(getSlotSizeDkv(database, i));
        }
    }
    return maxOffset + maxLength + HEADER_SIZE_KV;
}