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
void trySplitDKVSlot(KV* kv, unsigned int slotToSplit, unsigned int slotToSplitRequiredLength);
void SetSlotDKVAsOccupied(KV* kv, int emplacement_dkv);
void insertNewSlotDKV(KV* kv, int emplacement_a_decal);
int writeElementToKV(KV* kv, const kv_datum* key, const kv_datum* val, len_t offset);
void addsEntryToBLK(KV* kv, len_t emplacement_kv);

int kv_get(KV* kv, const kv_datum* key, kv_datum* val);
len_t lookupKeyOffset(KV* kv, const kv_datum* key);
int compareKeys(KV* kv, const kv_datum* key, len_t offset);
int fillValue(KV* kv, len_t offset, kv_datum* val, const kv_datum* key);

int kv_del(KV* kv, const kv_datum* key);
void freeSlotDKV(len_t offset, KV* kv);
void tryMergeSlots(KV* database, unsigned int centralSlot);
void mergeSlots(unsigned int left, unsigned int right, KV* database);
int freeEntryBLK(int blockIndex, len_t offset, KV* kv, int hash, int previousBloc);
int freeHashtableEntry(KV* kv, int hash);

void kv_start(KV* kv);

int kv_next(KV* kv, kv_datum* key, kv_datum* val);
int fillsKey(KV* kv, len_t offset, kv_datum* key);
len_t getKeyLengthFromKV(KV* kv, len_t offset);

int truncate_kv(KV* kv);

#endif