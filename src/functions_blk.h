#ifndef FUNCTIONS_BLK
#define FUNCTIONS_BLK

#include "commonFunctions.h"
#include "kv.h"
#include "structures.h"

// Adds an entry to BLK block by also updating the BLK block's number of slots counter.
// Returns 1 on success and -1 on error.
int addsEntryToBLK(KV* database, len_t offsetKV, unsigned int blockIndex);
// Frees the BLK block slot associated to offsetKV. Returns 1 on success and -1 on failure.
int freeEntryBLK(int blockIndex, int offset, KV* database, int hash, int previousBloc);
// Looks up whether a BLK block with a free entry is associated to the hash.
// Returns blockIndex when such a block is association to hash, -1 with EINVAL set when there is none and -1 upon error.
int getBLKBlockIndexWithFreeEntry(KV* database, int hash);
// Returns the index of first empty block. Assumes that an empty block exists.
int AllocatesNewBlock(KV* database);
// Looks up offset of key in KV in the BLK block blockIndex as well as next blocks of blockIndex.
// Returns the offset when found key, 0 with errno set to EINVAL if the key is not found, and 0 on error.
len_t lookupKeyOffsetKV(KV* database, const kv_datum* key, int blockIndex);

// Gets the index of the next BLK block of blockIndex. Returns the index on success and -1 on error.
int getIndexNextBLKBlock(unsigned int index, KV* database);
// Sets number of slots in BLK block blockIndex to nbSlots.
int setNbSlotsInBLKBlock(unsigned int nbSlots, unsigned int blockIndex, KV* database);
// Get number of slots in BLK block blockIndex.
int getNbSlotsInBLKBlock(unsigned int blockIndex, KV* database);
// Sets the KV offset of slot indexInBlock of BLK block blockIndex. Returns 1 on success and -1 on error.
int setKVOffsetInBLK(unsigned int offsetKV, unsigned int blockIndex, KV* database, unsigned int indexInBlock);
// Gets the KV offset of slot indexInBlock of BLK block blockIndex. Returns the offset when found and -1 on error.
int getKVOffsetBLKBlock(unsigned int indexInBlock, unsigned int blockIndex, KV* database);
// Returns whether BLK block blockIndex has a next block or not.
bool hasNextBLKBlock(unsigned int blockIndex, KV* database);
// Checks that blockIndex is loaded in database and if not loads it, by first writing the previous block to file.
int checkAndLoadBLKBlock(unsigned int blockIndex, KV* database);
// Get offset inside a BLK block
static inline len_t getOffsetBlk(unsigned int index) {
    return (index - 1) * BLOCK_SIZE + HEADER_SIZE_BLK;
}
// Get offset of BLK block
static inline len_t getOffsetBLKBlock(unsigned int index) {
    return HEADER_SIZE_BLOCK + index * sizeof(unsigned int);
}

#endif