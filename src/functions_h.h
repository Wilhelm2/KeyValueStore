#ifndef FUNCTIONS_H
#define FUNCTIONS_H

#include "commonFunctions.h"
#include "functions_blk.h"
#include "structures.h"

// Reads the index of the first BLK block containing the offset of keys whose offset is equal to hash.
// Returns block index on success, and -1 on failure, with errno set to EINVAL if no stored block index.
int readBLKBlockIndexOfHash(KV* database, unsigned int hash);
// Write BLK block index to H file. Returns 1 on success and -1 on failure.
int writeBLKBlockIndexToH(KV* database, int hash, int blockIndex);
// Frees the hashtable entry by replacing the block index stored at position hash with -1.
int freeHashtableEntry(KV* database, int hash);

// Gets the index of the first BLK block containing values for keys whose hash is equal to hash.
// Allocates a block and returns its index if no BLK block is associated to hash.
int getBlockIndexForHash(unsigned int hash, KV* database);

static inline len_t getOffsetH(unsigned int hash) {
    return HEADER_SIZE_H + hash * sizeof(unsigned int);
}
#endif