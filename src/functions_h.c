#include "functions_h.h"

int readBLKBlockIndexOfHash(KV* database, unsigned int hash) {
    int test;
    int blockIndex;
    if ((test = readAtPosition(database->fds.fd_h, getOffsetH(hash), &blockIndex, sizeof(int), database)) == -1)
        return -1;
    if (test == 0 || blockIndex == -1) {  // either reads 0 bytes or has no registered block
        errno = EINVAL;
        return -1;
    }
    return blockIndex;
}

int writeBLKBlockIndexToH(KV* database, int hash, int blockIndex) {
    if (writeAtPosition(database->fds.fd_h, getOffsetH(hash), &blockIndex, sizeof(unsigned int), database) == -1)
        return -1;
    return 1;
}

int freeHashtableEntry(KV* database, int hash) {
    return writeBLKBlockIndexToH(database, hash, -1);
}

int getBlockIndexForHash(unsigned int hash, KV* database) {
    int blockIndex;
    if ((blockIndex = getBLKBlockIndexWithFreeEntry(database, hash)) == -1 && errno == EINVAL) {
        blockIndex = AllocatesNewBlock(database);
        if (blockIndex == -1)
            return -1;
        if (writeBLKBlockIndexToH(database, hash, blockIndex) == -1)
            return -1;
    }
    return blockIndex;
}
