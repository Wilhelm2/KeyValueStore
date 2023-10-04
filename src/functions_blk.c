#include "functions_blk.h"

int addsEntryToBLK(KV* database, len_t offsetKV, unsigned int blockIndex) {
    int nbSlots = getNbSlotsInBLKBlock(blockIndex, database);
    if (nbSlots == -1)
        return -1;
    if (setKVOffsetInBLK(offsetKV, blockIndex, database, nbSlots) == -1)
        return -1;
    if (setNbSlotsInBLKBlock(nbSlots + 1, blockIndex, database) == -1)
        return -1;
    return 1;
}

int freeEntryBLK(int blockIndex, int offsetKV, KV* database, int hash, int previousblock) {
    do {
        int nbSlots = getNbSlotsInBLKBlock(blockIndex, database);
        if (nbSlots == -1)
            return -1;
        int offsetOfLastElementInblock = getKVOffsetBLKBlock(nbSlots - 1, blockIndex, database);
        if (offsetOfLastElementInblock == -1)
            return -1;
        for (int i = 0; i < nbSlots; i++) {
            if (offsetKV == getKVOffsetBLKBlock(i, blockIndex, database)) {  // found element
                // swaps the offset of the element with the one of the last element of the block
                setKVOffsetInBLK(offsetOfLastElementInblock, blockIndex, database, i);
                nbSlots--;
                setNbSlotsInBLKBlock(nbSlots, blockIndex, database);

                if (nbSlots == 0 && previousblock == 0 &&
                    hasNextBLKBlock(blockIndex, database) == false) {  // frees the block entry
                    if (freeHashtableEntry(database, hash) == -1)
                        return -1;
                    database->bh.blockIsOccupied[blockIndex] = false;  // marks the block as free
                    // printf("marks block %d as free\n", blockIndex);
                }
                return 1;
            }
        }
        // entry not found
        previousblock = blockIndex;
        blockIndex = getIndexNextBLKBlock(blockIndex, database);  // index of next block
        if (checkAndLoadBLKBlock(blockIndex, database) == -1)
            return 0;
    } while (true);
    return 1;
}

int getBLKBlockIndexWithFreeEntry(KV* database, int hash) {
    int blockIndex;
    bool hasNextBlock;

    blockIndex = readBLKBlockIndexOfHash(database, hash);
    if (blockIndex == -1)
        return -1;

    do {
        int nbElements = getNbSlotsInBLKBlock(blockIndex, database);
        if (nbElements == -1)
            return -1;
        if ((nbElements + 1) * sizeof(len_t) < BLOCK_SIZE - HEADER_SIZE_BLOCK)
            return blockIndex;  // Space for another slot
        hasNextBlock = hasNextBLKBlock(blockIndex, database);
        blockIndex = getIndexNextBLKBlock(blockIndex, database);
        if (blockIndex == -1)
            return -1;
    } while (hasNextBlock);

    errno = EINVAL;
    return -1;
}

int AllocatesNewBlock(KV* database) {
    unsigned int blockIndex = 0;
    while (true) {
        if (blockIndex == database->bh.blockIsOccupiedSize)  // increases the size of the array
        {
            database->bh.blockIsOccupiedSize += 100;
            database->bh.blockIsOccupied =
                realloc(database->bh.blockIsOccupied, database->bh.blockIsOccupiedSize * sizeof(bool));
            memset(database->bh.blockIsOccupied + database->bh.blockIsOccupiedSize - 100, false,
                   100 * sizeof(bool));  // sets new entries
        }
        if (database->bh.blockIsOccupied[blockIndex] == false)  // empty block
        {
            database->bh.blockIsOccupied[blockIndex] = true;
            database->bh.nb_blocks++;
            break;
        }
        blockIndex++;
    }
    return blockIndex;
}

len_t lookupKeyOffsetKV(KV* database, const kv_datum* key, int blockIndex) {
    int test;
    do {
        for (int i = 0; i < getNbSlotsInBLKBlock(blockIndex, database); i++) {
            test = compareKeys(database, key, getKVOffsetBLKBlock(i, blockIndex, database));
            if (test == -1)
                return 0;
            else if (test == 1)
                return getKVOffsetBLKBlock(i, blockIndex, database);
        }
        // loads new BLK block containing position of keys whose hash is equal to the hash of key.
        if (hasNextBLKBlock(blockIndex, database)) {
            blockIndex = getIndexNextBLKBlock(blockIndex, database);
            if (checkAndLoadBLKBlock(blockIndex, database) == -1)
                return 0;
        } else
            break;
    } while (true);

    errno = EINVAL;  // Key not found
    return 0;
}

int getIndexNextBLKBlock(unsigned int blockIndex, KV* database) {
    if (checkAndLoadBLKBlock(blockIndex, database) == -1)
        return -1;
    return *(int*)(database->bh.blockBLK + 1);
}

int setNbSlotsInBLKBlock(unsigned int nbSlots, unsigned int blockIndex, KV* database) {
    if (checkAndLoadBLKBlock(blockIndex, database) == -1)
        return -1;
    memcpy(database->bh.blockBLK + 5, &nbSlots, sizeof(unsigned int));
    return 1;
}

int getNbSlotsInBLKBlock(unsigned int blockIndex, KV* database) {
    if (checkAndLoadBLKBlock(blockIndex, database) == -1)
        return -1;
    return *(int*)(database->bh.blockBLK + 5);
}

int setKVOffsetInBLK(unsigned int offsetKV, unsigned int blockIndex, KV* database, unsigned int indexInBlock) {
    if (checkAndLoadBLKBlock(blockIndex, database) == -1)
        return -1;
    memcpy(database->bh.blockBLK + getOffsetBLKBlock(indexInBlock), &offsetKV, sizeof(len_t));
    return 1;
}

int getKVOffsetBLKBlock(unsigned int indexInBlock, unsigned int blockIndex, KV* database) {
    if (checkAndLoadBLKBlock(blockIndex, database) == -1)
        return -1;
    return *(int*)(database->bh.blockBLK + getOffsetBLKBlock(indexInBlock));
}

bool hasNextBLKBlock(unsigned int blockIndex, KV* database) {
    if (checkAndLoadBLKBlock(blockIndex, database) == -1)
        return false;
    return *(database->bh.blockBLK) > 0;
}

int checkAndLoadBLKBlock(unsigned int blockIndex, KV* database) {
    if (blockIndex == database->bh.indexCurrLoadedBlock)
        return 1;
    if (writeAtPosition(database->fds.fd_blk, getOffsetBlk(database->bh.indexCurrLoadedBlock), database->bh.blockBLK,
                        BLOCK_SIZE, database) == -1)
        return -1;
    memset(database->bh.blockBLK, 0, BLOCK_SIZE);
    database->bh.indexCurrLoadedBlock = blockIndex;
    if (readAtPosition(database->fds.fd_blk, getOffsetBlk(blockIndex), database->bh.blockBLK, BLOCK_SIZE, database) ==
        -1)
        return -1;
    return 1;
}