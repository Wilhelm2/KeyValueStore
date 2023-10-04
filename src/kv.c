// WILHELM Daniel YAMAN Kendal

#include "kv.h"

KV* kv_open(const char* dbname, const char* mode, int hashFunctionIndex, alloc_t alloc) {
    KV* kv = malloc(sizeof(KV));

    if (initializeFiles(kv, mode, dbname, hashFunctionIndex) == -1)
        return NULL;

    unsigned int hashIndex;
    if (readAtPosition(kv->fds.fd_h, 1, &hashIndex, sizeof(unsigned int), kv) == -1)
        return NULL;
    kv->hashFunction = getHashFunction(hashIndex);
    if (kv->hashFunction == NULL) {  // checks whether an hash function is associated to that index
        errno = EINVAL;
        return NULL;
    }

    if (readAtPosition(kv->fds.fd_blk, 1, &kv->bh.nb_blocks, sizeof(unsigned int), kv) == -1)
        return NULL;
    memset(kv->bh.blockBLK, 0, BLOCK_SIZE);
    // reads block 0 to avoid erasing it at next call of readNewBlock
    if (readAtPosition(kv->fds.fd_blk, getOffsetBlk(0), kv->bh.blockBLK, BLOCK_SIZE, kv) == -1)
        return NULL;
    kv->bh.indexCurrLoadedBlock = 0;

    // adds space for 40 new blocks
    kv->bh.blockIsOccupied = calloc(kv->bh.nb_blocks + 40, sizeof(bool));
    kv->bh.blockIsOccupiedSize = 40 + kv->bh.nb_blocks;
    if (readsBlockOccupiedness(kv) == -1) {
        kv_close(kv);
        return NULL;
    }

    len_t nbElementsInDKV;
    if (readAtPosition(kv->fds.fd_dkv, 1, &nbElementsInDKV, sizeof(int), kv) == -1)
        return NULL;
    // Adds space for 1000 elements
    kv->dkvh.dkv =
        calloc((nbElementsInDKV + 1000) * (sizeof(unsigned int) + sizeof(len_t)) + HEADER_SIZE_DKV, sizeof(char));
    if (readAtPosition(kv->fds.fd_dkv, 0, kv->dkvh.dkv,
                       nbElementsInDKV * (sizeof(unsigned int) + sizeof(len_t)) + HEADER_SIZE_DKV, kv) == -1)
        return NULL;
    kv->dkvh.maxElementsInDKV = nbElementsInDKV + 1000;

    kv->allocationMethod = alloc;
    kv->fds.mode = mode;
    return kv;
}

int kv_close(KV* kv) {
    int res = 0;

    if (strcmp(kv->fds.mode, "r") != 0) {
        // Writes DKV to file
        if (writeAtPosition(kv->fds.fd_dkv, 0, kv->dkvh.dkv, sizeOfDKVFilled(kv), kv) == -1) {
            freeDatabase(kv);
            return -1;
        }

        // Writes the number of blocks
        if (writeAtPosition(kv->fds.fd_blk, 1, &kv->bh.nb_blocks, sizeof(unsigned int), kv) == -1) {
            freeDatabase(kv);
            return -1;
        }

        // Writes last updated block to blk
        if (writeAtPosition(kv->fds.fd_blk, getOffsetBlk(kv->bh.indexCurrLoadedBlock), &kv->bh.blockBLK, BLOCK_SIZE,
                            kv) == -1) {
            freeDatabase(kv);
            return -1;
        }

        // Truncates dkv to remove free slots at the end
        if (ftruncate(kv->fds.fd_dkv, sizeOfDKVFilled(kv)) == -1) {
            perror("ftruncate dkv");
            exit(1);
        }
    }

    if (truncate_kv(kv) == -1)
        res = -1;
    res |= closeFileDescriptors(kv);
    freeDatabase(kv);
    return (res == 0 ? 1 : -1);
}

int kv_put(KV* kv, const kv_datum* key, const kv_datum* val) {
    int hash = kv->hashFunction(key);
    int blockIndex;
    if (strcmp(kv->fds.mode, "r") == 0) {  // Checks rights to write
        errno = EACCES;
        return -1;
    }

    // Deletes the key and its associated value if the key is already in the database
    if (kv_del(kv, key) == -1 && !(errno == EINVAL || errno == ENOENT))
        return -1;

    if ((blockIndex = getBlockIndexForHash(hash, kv)) == -1)
        return -1;

    unsigned int dkvSlot = allocatesDKVSlot(kv, key, val);  // Allocates DKV slot and Updates DKV

    if (writeElementToKV(kv, key, val, getKVOffsetDKVSlot(kv, dkvSlot)) == -1)  // Writes tuple to KV
        return -1;
    if (addsEntryToBLK(kv, getKVOffsetDKVSlot(kv, dkvSlot), blockIndex) == -1)  // Writes the entry into BLK
        return -1;
    return 0;
}

int kv_get(KV* kv, const kv_datum* key, kv_datum* val) {
    unsigned int hash = kv->hashFunction(key);
    int blockIndex;
    int offset;
    if (key == NULL || key->ptr == NULL || val == NULL || key->len < 1) {
        errno = EINVAL;
        return -1;
    }
    if (strcmp(kv->fds.mode, "w") == 0) {  // not allowed to read
        errno = EACCES;
        return -1;
    }

    blockIndex = readBLKBlockIndexOfHash(kv, hash);
    if (blockIndex == -1)
        return (errno == EINVAL ? 0 : -1);

    // Lookup the key in KV. Returns offset when found
    offset = lookupKeyOffsetKV(kv, key, blockIndex);
    if (offset == 0)
        return 0;
    if (readValue(kv, offset, val, key) == -1)
        return -1;
    return 1;
}

int kv_del(KV* kv, const kv_datum* key) {
    unsigned int hash = kv->hashFunction(key);
    int blockIndex;
    len_t offset;
    if (strcmp(kv->fds.mode, "r") == 0) {  // checks write permission
        errno = EACCES;
        return -1;
    }

    blockIndex = readBLKBlockIndexOfHash(kv, hash);
    if (blockIndex == -1)
        return (errno == EINVAL ? 0 : -1);

    offset = lookupKeyOffsetKV(kv, key, blockIndex);
    if (offset == 0 && errno != EINVAL)
        return -1;
    else if (offset == 0) {  // key not contained
        errno = ENOENT;
        return -1;
    }
    if (freeEntryBLK(blockIndex, offset, kv, hash, 0) == -1)
        return -1;
    freeDKVSlot(offset, kv);
    return 0;
}

void kv_start(KV* kv) {
    if (strcmp(kv->fds.mode, "w") == 0) {  // no rights to read
        errno = EACCES;
    }
    lseek(kv->fds.fd_kv, HEADER_SIZE_KV, SEEK_SET);
    kv->dkvh.nextTuple = 0;  // initializes the number of read couples to 0
}

int kv_next(KV* kv, kv_datum* key, kv_datum* val) {
    if (strcmp(kv->fds.mode, "w") == 0) {
        errno = EACCES;
        return -1;
    }
    while (kv->dkvh.nextTuple < getNbSlotsInDKV(kv) && getDKVSlotSize(kv, kv->dkvh.nextTuple) > 0)
        kv->dkvh.nextTuple++;  // jumps empty slots

    if (getNbSlotsInDKV(kv) == kv->dkvh.nextTuple)  // Arrived at the end of the database.
        return 0;

    len_t tupleOffset = getKVOffsetDKVSlot(kv, kv->dkvh.nextTuple);
    if (readKey(kv, tupleOffset, key) == -1)
        return -1;
    if (readValue(kv, tupleOffset, val, key) == -1)
        return -1;
    kv->dkvh.nextTuple++;
    return 1;
}

int truncate_kv(KV* kv) {
    if (strcmp(kv->fds.mode, "r") == 0)  // don't truncate since kv wasn't changed
        return 1;
    if (ftruncate(kv->fds.fd_kv, getKVSize(kv)) == -1)
        return -1;
    return 1;
}
