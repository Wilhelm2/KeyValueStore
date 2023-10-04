#ifndef FUNCTIONS_DKV
#define FUNCTIONS_DKV

#include "commonFunctions.h"
#include "slotAllocations.h"
#include "structures.h"

int allocatesDkvSlot(KV* database, const kv_datum* key, const kv_datum* val);
void createNewSlotEndDKV(KV* kv, const kv_datum* key, const kv_datum* val);
void increaseSizeDkv(KV* kv);
void SetSlotDKVAsOccupied(KV* kv, int dkvSlot);
void trySplitDKVSlot(KV* kv, unsigned int slotToSplit, unsigned int slotToSplitRequiredLength);
void insertNewSlotDKV(KV* kv, int firstSlot);

// Looks up the slot of DKV pointing to offsetKV and frees it.
void freeDKVSlot(len_t offsetKV, KV* kv);
// Tries to merge centralSlot (which is free) with the left and right slot provided they are free too.
void trySlotMerge(KV* database, unsigned int centralSlot);
// Merges the DKV slots left and right.
void mergeSlots(unsigned int left, unsigned int right, KV* database);
// Returns the size of KV.
len_t getKVSize(KV* database);

#endif