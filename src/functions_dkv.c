#include "functions_dkv.h"

int allocatesDKVSlot(KV* database, const kv_datum* key, const kv_datum* val) {
    int dkvSlot;
    if ((dkvSlot = getAllocationMethod(database)(database, key, val)) == -1) {  // no free slot, so creates one
        createNewSlotEndDKV(database, key, val);
        dkvSlot = getNbSlotsInDKV(database) - 1;
    }
    SetDKVSlotAsOccupied(database, dkvSlot);
    trySplitDKVSlot(database, dkvSlot, key->len + sizeof(key->len) + val->len + sizeof(val->len));
    return dkvSlot;
}

void createNewSlotEndDKV(KV* kv, const kv_datum* key, const kv_datum* val) {
    len_t offsetNewSlot;
    if (getNbSlotsInDKV(kv) == kv->dkvh.maxElementsInDKV)
        increaseDKVSize(kv);

    len_t lengthNewSlot = sizeof(len_t) + key->len + sizeof(len_t) + val->len;
    memcpy(kv->dkvh.dkv + getOffsetDkv(getNbSlotsInDKV(kv)), &lengthNewSlot, sizeof(len_t));
    if (getNbSlotsInDKV(kv) == 0)
        offsetNewSlot = HEADER_SIZE_KV;
    else
        offsetNewSlot =
            getKVOffsetDKVSlot(kv, getNbSlotsInDKV(kv) - 1) + abs(getDKVSlotSize(kv, getNbSlotsInDKV(kv) - 1));
    memcpy(kv->dkvh.dkv + getOffsetDkv(getNbSlotsInDKV(kv)) + sizeof(len_t), &offsetNewSlot, sizeof(len_t));
    setNbSlotsInDKV(kv, getNbSlotsInDKV(kv) + 1);
}

void increaseDKVSize(KV* database) {
    database->dkvh.maxElementsInDKV += 1000;
    database->dkvh.dkv = realloc(database->dkvh.dkv, getMaxDKVSize(database));
}

void SetDKVSlotAsOccupied(KV* kv, int dkvSlot) {
    int length = getDKVSlotSize(kv, dkvSlot) * (-1);
    memcpy(kv->dkvh.dkv + getOffsetDkv(dkvSlot), &length, sizeof(int));
}

void trySplitDKVSlot(KV* kv, unsigned int slotToSplit, unsigned int slotToSplitRequiredLength) {
    len_t remainingBytes = abs(getDKVSlotSize(kv, slotToSplit)) - slotToSplitRequiredLength;
    if (remainingBytes > (2 * sizeof(unsigned int) + 2)) {
        // Size for another slot of at least one character for key and value
        insertNewDKVSlot(kv, slotToSplit);
        setDKVSlotSize(kv, slotToSplit, slotToSplitRequiredLength * (-1));  // *-1 to indicate that the slot is taken
        setDKVSlotSize(kv, slotToSplit + 1, remainingBytes);
        setKVOffsetDKVSlot(kv, slotToSplit + 1, getKVOffsetDKVSlot(kv, slotToSplit) + slotToSplitRequiredLength);
    }
}

void insertNewDKVSlot(KV* database, int firstSlot) {
    if (getNbSlotsInDKV(database) == database->dkvh.maxElementsInDKV)  // adds space to dkv
        increaseDKVSize(database);
    memcpy(database->dkvh.dkv + getOffsetDkv(firstSlot + 1), database->dkvh.dkv + getOffsetDkv(firstSlot),
           sizeOfDKVFilled(database) - getOffsetDkv(firstSlot));
    setNbSlotsInDKV(database, getNbSlotsInDKV(database) + 1);
}

void freeDKVSlot(len_t offsetKV, KV* kv) {
    for (unsigned int i = 0; i < getNbSlotsInDKV(kv); i++) {
        if (getKVOffsetDKVSlot(kv, i) == offsetKV) {
            int length = (-1) * getDKVSlotSize(kv, i);  // multiplies by -1 to tag the slot as free
            memcpy(kv->dkvh.dkv + getOffsetDkv(i), &length, sizeof(int));
            trySlotMerge(kv, i);           // Tries to merge the now free slot with right and left slots
            if (i == getNbSlotsInDKV(kv))  // the newly freed slot is the last one of DKV
                setNbSlotsInDKV(kv, getNbSlotsInDKV(kv) - 1);
            return;
        }
    }
}

void trySlotMerge(KV* database, unsigned int centralSlot) {
    if (centralSlot > 0) {                                    // has slot to the left
        if (getDKVSlotSize(database, centralSlot - 1) > 0) {  // left slot is free -> merge
            mergeSlots(centralSlot - 1, centralSlot, database);
            centralSlot--;  // because has just removed the left slot
        }
    }
    if (centralSlot < getNbSlotsInDKV(database) - 1) {      // has slot to the right
        if (getDKVSlotSize(database, centralSlot + 1) > 0)  // right slot is free -> merge
            mergeSlots(centralSlot, centralSlot + 1, database);
    }
}

void mergeSlots(unsigned int left, unsigned int right, KV* database) {
    printf("merge slots %d and %d total number of slots %d\n", left, right, getNbSlotsInDKV(database));
    printf("offset %d and %d\n", getKVOffsetDKVSlot(database, left), getKVOffsetDKVSlot(database, right));
    printf("length %d and %d\n", getDKVSlotSize(database, left), getDKVSlotSize(database, right));
    len_t newSlotSize = getDKVSlotSize(database, left) + getDKVSlotSize(database, right);
    len_t newSlotOffset = getKVOffsetDKVSlot(database, left);
    memcpy(database->dkvh.dkv + getOffsetDkv(left), database->dkvh.dkv + getOffsetDkv(right),
           sizeOfDKVFilled(database) - getOffsetDkv(right));  // Moves dkv one to the left to remove right slot
    memcpy(database->dkvh.dkv + getOffsetDkv(left), &newSlotSize, sizeof(len_t));
    memcpy(database->dkvh.dkv + getOffsetDkv(left) + sizeof(int), &newSlotOffset, sizeof(len_t));
    setNbSlotsInDKV(database, getNbSlotsInDKV(database) - 1);
}

len_t getKVSize(KV* database) {
    len_t maxOffset = 0;
    len_t maxLength = 0;
    unsigned int nbElementsInDKV = getNbSlotsInDKV(database);
    for (unsigned int i = 0; i < nbElementsInDKV; i++) {
        if (maxOffset <= getKVOffsetDKVSlot(database, i)) {
            maxOffset = getKVOffsetDKVSlot(database, i);
            maxLength = abs(getDKVSlotSize(database, i));
        }
    }
    return maxOffset + maxLength + HEADER_SIZE_KV;
}