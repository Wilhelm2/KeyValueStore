#ifndef FUNCTIONS_DKV
#define FUNCTIONS_DKV

#include "commonFunctions.h"
#include "slotAllocations.h"
#include "structures.h"

// Allocates a new DKV slot big enough to contain the couple (key,val). Returns the DKV slot index.
int allocatesDKVSlot(KV* database, const kv_datum* key, const kv_datum* val);
// Creates new DKV slot at the end of DKV and initializes it to contain (key,val).
void createNewSlotEndDKV(KV* kv, const kv_datum* key, const kv_datum* val);
// Increases the size of DKV by adding space for 1000 new slots.
void increaseDKVSize(KV* kv);
// Sets DKV slot as occupied by multiplying its length by -1.
void SetDKVSlotAsOccupied(KV* kv, int dkvSlot);
// Splits the DKV slot in two if there is enough space left for another slot.
void trySplitDKVSlot(KV* kv, unsigned int slotToSplit, unsigned int slotToSplitRequiredLength);
// Shifts DKV slots one to the right beginning from firstSlot.
void insertNewDKVSlot(KV* kv, int firstSlot);
// Looks up the DKV slot pointing to offsetKV and frees it.
void freeDKVSlot(len_t offsetKV, KV* kv);
// Tries to merge centralSlot (which is free) with the left and right slot provided they are free too.
void trySlotMerge(KV* database, unsigned int centralSlot);
// Merges the DKV slots left and right.
void mergeSlots(unsigned int left, unsigned int right, KV* database);
// Returns the size of KV.
len_t getKVSize(KV* database);

// Gets the offset in DKV of slot index.
static inline len_t getOffsetDkv(unsigned int index) {
    return HEADER_SIZE_DKV + index * 2 * sizeof(unsigned int);
}

// Sets the DKV slot size of DKV slot index to size.
static inline void setDKVSlotSize(KV* kv, unsigned int index, unsigned int size) {
    memcpy(kv->dkvh.dkv + getOffsetDkv(index), &size, sizeof(unsigned int));
}

// Returns the DKV slot size of the DKV slot index.
// WARNING: occupied slots have a size < 0 (*(-1) to get the slot size), while free slots have a size > 0.
static inline int getDKVSlotSize(KV* kv, unsigned int index) {
    return *((int*)(kv->dkvh.dkv + getOffsetDkv(index)));  // keeps as int since < 0 means that the slot is not free
}

// Sets the KV offset stored at the DKV slot index to offsetKV.
static inline void setKVOffsetDKVSlot(KV* kv, unsigned int index, unsigned int offsetKV) {
    memcpy(kv->dkvh.dkv + getOffsetDkv(index) + sizeof(unsigned int), &offsetKV, sizeof(unsigned int));
}

// Returns the KV offset of the slot index of DKV.
static inline unsigned int getKVOffsetDKVSlot(KV* kv, unsigned int index) {
    return *((int*)(kv->dkvh.dkv + getOffsetDkv(index) + sizeof(unsigned int)));
}

// Returns the number of used DKV slots.
static inline unsigned int getNbSlotsInDKV(KV* kv) {
    return *(int*)(kv->dkvh.dkv + 1);
}

// Sets the number of used DKV slots.
static inline void setNbSlotsInDKV(KV* kv, unsigned int nbSlots) {
    memcpy(kv->dkvh.dkv + 1, &nbSlots, sizeof(unsigned int));
}

// Returns the currently used size of DKV.
static inline unsigned int sizeOfDKVFilled(KV* kv) {
    return getNbSlotsInDKV(kv) * (sizeof(unsigned int) + sizeof(len_t)) + HEADER_SIZE_DKV;
}

// Returns the actual maximal size of DKV.
static inline unsigned int getMaxDKVSize(KV* kv) {
    return kv->dkvh.maxElementsInDKV * (sizeof(unsigned int) + sizeof(len_t)) + HEADER_SIZE_DKV;
}

#endif