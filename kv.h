#ifndef KV_H
#define KV_H

#include "hashFunctions.h"
#include "kvInitialization.h"
#include "slotAllocations.h"
#include "structures.h"

int kv_close(KV* kv);
void freeDatabase(KV* database);

int kv_put(KV* kv, const kv_datum* key, const kv_datum* val);
int getBlockIndexForHash(unsigned int hash, KV* kv);
int getBlockIndexWithFreeEntry(KV* kv, int hash);
int AllocatesNewBlock(KV* kv);
int writeBlockIndexToH(KV* kv, int hash, int blockIndex);
int allocatesDkvSlot(KV* database, const kv_datum* key, const kv_datum* val);
void createNewSlotEndDKV(KV* kv, const kv_datum* key, const kv_datum* val);
void increaseSizeDkv(KV* kv);
void SetSlotDKVAsOccupied(KV* kv, int dkvSlot);
void trySplitDKVSlot(KV* kv, unsigned int slotToSplit, unsigned int slotToSplitRequiredLength);
void insertNewSlotDKV(KV* kv, int firstSlot);
int writeElementToKV(KV* kv, const kv_datum* key, const kv_datum* val, len_t offsetKV);
int addsEntryToBLK(KV* kv, len_t offsetKV, unsigned int blockIndex);

int kv_get(KV* kv, const kv_datum* key, kv_datum* val);
len_t lookupKeyOffsetKV(KV* kv, const kv_datum* key, int blockIndex);
int compareKeys(KV* kv, const kv_datum* key, len_t offsetKV);
int fillValue(KV* kv, len_t offsetKV, kv_datum* val, const kv_datum* key);

int kv_del(KV* kv, const kv_datum* key);
void freeSlotDKV(len_t offsetKV, KV* kv);
void tryMergeSlots(KV* database, unsigned int centralSlot);
void mergeSlots(unsigned int left, unsigned int right, KV* database);
int freeEntryBLK(int blockIndex, int offset, KV* kv, int hash, int previousBloc);
int freeHashtableEntry(KV* kv, int hash);

void kv_start(KV* kv);

int kv_next(KV* kv, kv_datum* key, kv_datum* val);
int fillsKey(KV* kv, len_t offsetKV, kv_datum* key);
len_t getKeyLengthFromKV(KV* kv, len_t offsetKV);

int truncate_kv(KV* kv);

void verifyEntriesDKV(KV* database);
#endif