// WILHELM Daniel YAMAN Kendal

#include "kv.h"

// Closes the database. Returns -1 when fails and 1 on success
int kv_close(KV* kv) {
    int res = 1;

    if (strcmp(kv->fds.mode, "r") != 0) {
        if (writeAtPosition(kv->fds.fd_dkv, 0, kv->dkvh.dkv, sizeOfDKVFilled(kv), kv) == -1) {
            freeDatabase(kv);
            return -1;
        }

        if (writeAtPosition(kv->fds.fd_blk, 1, &kv->bh.nb_blocks, sizeof(unsigned int), kv) == -1) {
            freeDatabase(kv);
            return -1;
        }
        if (ftruncate(kv->fds.fd_dkv, sizeOfDKVFilled(kv)) == -1) {
            perror("ftruncate dkv");
            exit(1);
        }
    }

    if (truncate_kv(kv) == -1)
        res = -1;
    res |= closeFileDescriptors(kv);
    freeDatabase(kv);
    return res;
}

void freeDatabase(KV* database) {
    free(database->dkvh.dkv);
    free(database->bh.blockIsOccupied);
    free(database);
}

int kv_put(KV* kv, const kv_datum* key, const kv_datum* val) {
    int hash = kv->hashFunction(key);
    int blockIndex;
    if (strcmp(kv->fds.mode, "r") == 0) {  // Checks rights to write
        errno = EACCES;
        return -1;
    }

    if (kv_del(kv, key) == -1 && !(errno == EINVAL || errno == ENOENT))  // Deletes the key if already present
        return -1;

    // First gets a block
    if ((blockIndex = getBlockIndexForHash(hash, kv)) == -1)
        return -1;

    unsigned int dkvSlot = allocatesDkvSlot(kv, key, val);  // Updates DKV

    if (writeElementToKV(kv, key, val, access_offset_dkv(dkvSlot, kv)) == -1)  // Writes tuple to KV
        return -1;
    addsEntryToBLK(kv, access_offset_dkv(dkvSlot, kv));
    return 0;
}

int getBlockIndexForHash(unsigned int hash, KV* kv) {
    int blockIndex;
    // First looks the block up. The function returns 0 when the hash is associated to no block
    if ((blockIndex = getBlockIndexWithFreeEntry(kv, hash)) == 0) {
        // Allocate a block to the hash value
        blockIndex = AllocatesNewBlock(kv);
        if (blockIndex == -1)
            return -1;
        if (writeBlockIndexToH(kv, hash, blockIndex) == -1)
            return -1;
    }
    return blockIndex;
}

// Lookup whether a block with a free entry is associated to hash
// Returns blockIndex when there is, 0 when there is none and -1 upon error
int getBlockIndexWithFreeEntry(KV* kv, int hash) {
    int test;
    int blockIndex;

    // Reads the block index contained at the hash position
    if ((test = readAtPosition(kv->fds.fd_h, getOffsetH(hash), &blockIndex, sizeof(int), kv)) == -1)
        return -1;
    if (test == 0 || blockIndex == -1) {  // either read 0 bytes or has no registered block
        return 0;
    }
    while (blockIndex >= 0) {
        // Reads the block to look if it has free slots
        if (readNewBlock(blockIndex, kv) == -1)
            return -1;
        if ((getNbElementsInBlock(kv->bh.block) + 1) * sizeof(len_t) < BLOCK_SIZE - LG_EN_TETE_BLOCK)
            return blockIndex;  // Space for another slot
        blockIndex = getIndexNextBlock(kv->bh.block);
    }
    return 0;
}

// Returns the index of first empty block. Assumes that an empty block exists
int AllocatesNewBlock(KV* kv) {
    unsigned int blockIndex = 0;
    while (true) {
        if (blockIndex == kv->bh.blockIsOccupiedSize)  // increases the size of the array
        {
            kv->bh.blockIsOccupiedSize += 100;
            kv->bh.blockIsOccupied = realloc(kv->bh.blockIsOccupied, kv->bh.blockIsOccupiedSize * sizeof(bool));
            memset(kv->bh.blockIsOccupied + kv->bh.blockIsOccupiedSize - 100, false,
                   100 * sizeof(bool));  // sets new entries
        }
        if (kv->bh.blockIsOccupied[blockIndex] == false)  // empty block
        {
            kv->bh.blockIsOccupied[blockIndex] = true;
            kv->bh.nb_blocks++;
            break;
        }
        blockIndex++;
    }
    return blockIndex;
}

// Write block index to H file
// Returns 1 on success and -1 on failure
int writeBlockIndexToH(KV* kv, int hash, int blockIndex) {
    if (writeAtPosition(kv->fds.fd_h, getOffsetH(hash), &blockIndex, sizeof(unsigned int), kv) == -1)
        return -1;
    return 1;
}

int allocatesDkvSlot(KV* database, const kv_datum* key, const kv_datum* val) {
    int dkvSlot;
    if ((dkvSlot = choix_allocation(database)(database, key, val)) == -1) {  // no free slot, so creates one
        createNewSlotEndDKV(database, key, val);
        dkvSlot = getSlotsInDKV(database) - 1;
    }
    SetSlotDKVAsOccupied(database, dkvSlot);
    trySplitDKVSlot(database, dkvSlot, key->len + sizeof(key->len) + val->len + sizeof(val->len));
    return dkvSlot;
}

// Creates new slot at the end of DKV
void createNewSlotEndDKV(KV* kv, const kv_datum* key, const kv_datum* val) {
    len_t offsetNewSlot;

    if (getSlotsInDKV(kv) == kv->dkvh.maxElementsInDKV)
        increaseSizeDkv(kv);

    len_t lengthNewSlot = sizeof(len_t) + key->len + sizeof(len_t) + val->len;
    memcpy(kv->dkvh.dkv + getOffsetDkv(getSlotsInDKV(kv)), &lengthNewSlot, sizeof(len_t));
    if (getSlotsInDKV(kv) == 0)
        offsetNewSlot = LG_EN_TETE_KV;
    else
        offsetNewSlot = access_offset_dkv(getSlotsInDKV(kv) - 1, kv) + abs(access_lg_dkv(getSlotsInDKV(kv) - 1, kv));
    memcpy(kv->dkvh.dkv + getOffsetDkv(getSlotsInDKV(kv)) + sizeof(len_t), &offsetNewSlot, sizeof(len_t));
    setSlotsInDKV(kv, getSlotsInDKV(kv) + 1);
}

// Increases the size of dkv
void increaseSizeDkv(KV* database) {
    database->dkvh.maxElementsInDKV += 1000;
    database->dkvh.dkv = realloc(database->dkvh.dkv, sizeOfDKVMax(database));
}

// Sets DKV slot as occupied by multiplying its length by -1
void SetSlotDKVAsOccupied(KV* kv, int dkvSlot) {
    int length = access_lg_dkv(dkvSlot, kv) * (-1);
    memcpy(kv->dkvh.dkv + getOffsetDkv(dkvSlot), &length, sizeof(int));
}

// Splits the DKV slot in two if there is enough space to contain another slot
void trySplitDKVSlot(KV* kv, unsigned int slotToSplit, unsigned int slotToSplitRequiredLength) {
    len_t remainingBytes = (-1) * access_lg_dkv(slotToSplit, kv) - slotToSplitRequiredLength;
    if (remainingBytes > (2 * sizeof(unsigned int) + 2)) {
        // Size for another slot of at least one character for key and value
        insertNewSlotDKV(kv, slotToSplit);
        setSlotSizeDkv(kv, slotToSplit + 1, remainingBytes);
        setSlotOffsetDkv(kv, slotToSplit + 1, getSlotOffsetDkv(kv, slotToSplit) + slotToSplitRequiredLength);
        setSlotSizeDkv(kv, slotToSplit, slotToSplitRequiredLength * (-1));  //*-1 to indicate that slot is taken
    }
}

// Shifts slots one to the right
void insertNewSlotDKV(KV* database, int firstSlot) {
    if (getSlotsInDKV(database) == database->dkvh.maxElementsInDKV)  // adds space to dkv
        increaseSizeDkv(database);

    memcpy(database->dkvh.dkv + getOffsetDkv(firstSlot), database->dkvh.dkv + getOffsetDkv(firstSlot + 1),
           sizeOfDKVFilled(database) - getOffsetDkv(firstSlot + 1));
    setSlotsInDKV(database, getSlotsInDKV(database) + 1);
}

int writeElementToKV(KV* kv, const kv_datum* key, const kv_datum* val, len_t offsetKV) {
    // First writes into an array to do only one write
    unsigned int nbBytesToWrite = sizeof(len_t) + key->len + sizeof(len_t) + val->len;
    unsigned char* tabwrite = malloc(nbBytesToWrite);
    // Writes key length + key
    memcpy(tabwrite, &key->len, sizeof(len_t));
    memcpy(tabwrite + sizeof(len_t), key->ptr, key->len);
    // Writes value length + value
    memcpy(tabwrite + sizeof(len_t) + key->len, &val->len, sizeof(len_t));
    memcpy(tabwrite + sizeof(len_t) + key->len + sizeof(len_t), val->ptr, val->len);

    if (writeAtPosition(kv->fds.fd_kv, offsetKV, tabwrite, nbBytesToWrite, kv) == -1)
        return -1;

    free(tabwrite);
    return 1;
}

// Writes KV offset to BLK
void addsEntryToBLK(KV* kv, len_t offsetKV) {
    unsigned int nbSlots = getNbElementsInBlock(kv->bh.block);
    printf("writes offset to blk %d\n", offsetKV);
    memcpy(kv->bh.block + getOffsetBlock(nbSlots), &offsetKV, sizeof(len_t));
    setNbElementsInBlock(kv->bh.block, nbSlots + 1);
}

// Lookup the value associated to key
// Returns 1 when found, 0 when not found, and -1 when error
int kv_get(KV* kv, const kv_datum* key, kv_datum* val) {
    unsigned int hash = kv->hashFunction(key);
    int blockIndex, readTest;
    int offset;
    if (key == NULL || key->ptr == NULL || val == NULL) {
        errno = EINVAL;
        return -1;
    }
    if (strcmp(kv->fds.mode, "w") == 0) {  // not allowed to read
        errno = EACCES;
        return -1;
    }

    if ((readTest = readAtPosition(kv->fds.fd_h, getOffsetH(hash), &blockIndex, sizeof(unsigned int), kv)) == -1)
        return -1;
    else if (readTest == 0 || blockIndex == -1) {  // either no block associated to that hash or error
        errno = EINVAL;
        return 0;
    }
    if (((unsigned int)blockIndex) != kv->bh.indexCurrLoadedBlock) {
        if (readNewBlock(blockIndex, kv) == -1)
            return -1;
    }

    // Lookup the key in KV. Returns offset when found
    offset = lookupKeyOffsetKV(kv, key);
    printf("hash %d blockIndex %d readTest %d offset %d\n", hash, blockIndex, readTest, offset);
    if (offset == 0)
        return 0;
    if (fillValue(kv, offset, val, key) == -1)
        return -1;
    return 1;
}

// Lookup the offset of key key
// Returns offset when found, -1 when error, and 0 when not found
len_t lookupKeyOffsetKV(KV* kv, const kv_datum* key) {
    int test;
    bool nextBlock;
    do {
        for (unsigned int i = 0; i < getNbElementsInBlock(kv->bh.block); i++) {
            test = compareKeys(kv, key, *(int*)(kv->bh.block + getOffsetBlock(i)));
            printf("checks element %d cmp result %d\n", i, test);
            if (test == -1)
                return 0;
            else if (test == 1)
                return *(int*)(kv->bh.block + getOffsetBlock(i));
        }
        if (hasNextBlock(kv->bh.block)) {
            nextBlock = hasNextBlock(kv->bh.block);
            if (readNewBlock(getIndexNextBlock(kv->bh.block), kv) == -1)
                return 0;
        }
    } while (nextBlock);

    errno = EINVAL;  // Key not found
    return 0;
}

// Returns 1 when keys are equal, -1 when error, and 0 otherwise
int compareKeys(KV* kv, const kv_datum* key, len_t offsetKV) {
    len_t keyLength = 0;
    len_t totalReadBytes = 0;
    int readBytes = 0;
    unsigned char buf[2048];
    unsigned int maxReadBytesInStep = 0;

    if (readAtPosition(kv->fds.fd_kv, offsetKV, &keyLength, sizeof(int), kv) == -1)
        return -1;

    if (keyLength != key->len)
        return 0;
    while (totalReadBytes < keyLength) {
        maxReadBytesInStep = (keyLength - totalReadBytes) % 2049;  // reads at most 2048 bytes at each step
        readBytes = read_controle(kv->fds.fd_kv, buf, maxReadBytesInStep);
        if (readBytes == -1)
            return -1;
        printf("length %d key %.*s\n", readBytes, readBytes, buf);

        if (memcmp(((unsigned char*)key->ptr) + totalReadBytes, buf, readBytes) != 0)
            return 0;  // part of keys are not equal
        totalReadBytes += (len_t)readBytes;
    }
    return 1;
}

// Fills data into val
// Returns 1 on success and -1 on failure
int fillValue(KV* database, len_t offsetKV, kv_datum* val, const kv_datum* key) {
    if (val == NULL || key == NULL) {
        errno = EINVAL;
        return -1;
    }
    unsigned int posVal = offsetKV + sizeof(len_t) + key->len;
    if (readAtPosition(database->fds.fd_kv, posVal, &val->len, sizeof(len_t), database) == -1)
        return -1;
    val->ptr = malloc(val->len);
    if (readAtPosition(database->fds.fd_kv, posVal + sizeof(len_t), val->ptr, val->len, database) == -1)
        return -1;
    return 1;
}

int kv_del(KV* kv, const kv_datum* key) {
    unsigned int hash = kv->hashFunction(key);
    unsigned int blockIndex;
    len_t offset;
    int test;
    if (strcmp(kv->fds.mode, "r") == 0) {  // verifies write permission
        errno = EACCES;
        return -1;
    }

    if ((test = readAtPosition(kv->fds.fd_h, getOffsetH(hash), &blockIndex, sizeof(blockIndex), kv)) == -1) {
        if (errno == EINVAL)
            return 0;
        return -1;
    }

    if (test == 0 || blockIndex == 0) {  // no registered block for this hash
        errno = ENOENT;
        return -1;
    }

    if (readNewBlock(blockIndex, kv) == -1)
        return -1;

    offset = lookupKeyOffsetKV(kv, key);
    if (offset == 0 && errno != EINVAL)
        return -1;
    else if (offset == 0) {  // key not contained
        errno = ENOENT;
        return -1;
    }

    freeSlotDKV(offset, kv);
    if (freeEntryBLK(blockIndex, offset, kv, hash, 0) == -1)
        return -1;
    return 0;
}

// Frees an entry of DKV identified by its offset in KV
void freeSlotDKV(len_t offsetKV, KV* kv) {
    for (unsigned int i = 0; i < getSlotsInDKV(kv); i++) {
        if (access_offset_dkv(i, kv) == offsetKV) {
            int length = (-1) * access_lg_dkv(i, kv);  // multiplies by -1 to tag the slot as free
            memcpy(kv->dkvh.dkv + getOffsetDkv(i), &length, sizeof(int));
            tryMergeSlots(kv, i);
            if (i == (getSlotsInDKV(kv) - 1))  // the newly freed slot is the last one of DKV
                setSlotsInDKV(kv, getSlotsInDKV(kv) - 1);
            return;
        }
    }
}

void tryMergeSlots(KV* database, unsigned int centralSlot) {
    if (centralSlot > 0) {
        if (access_lg_dkv(centralSlot - 1, database) > 0) {  // free slot -> merge
            mergeSlots(centralSlot - 1, centralSlot, database);
            centralSlot--;  // since just removed the left slot
        }
    }
    if (centralSlot < getSlotsInDKV(database) - 1) {
        if (access_lg_dkv(centralSlot + 1, database) > 0)  // free slot -> merge
            mergeSlots(centralSlot, centralSlot + 1, database);
    }
}

void mergeSlots(unsigned int left, unsigned int right, KV* database) {
    len_t newSize = access_lg_dkv(left, database) + access_lg_dkv(right, database);
    memcpy(database->dkvh.dkv + getOffsetDkv(left), database->dkvh.dkv + getOffsetDkv(right),
           sizeOfDKVFilled(database) - getOffsetDkv(right));
    memcpy(database->dkvh.dkv + getOffsetDkv(left), &newSize, sizeof(len_t));
    setSlotsInDKV(database, getSlotsInDKV(database) - 1);
}

// Frees an entry in blk. Returns 1 on success and -1 on failure
int freeEntryBLK(int blockIndex, len_t offsetKV, KV* kv, int hash, int previousblock) {
    do {
        unsigned int nbSlots = getNbElementsInBlock(kv->bh.block);
        unsigned int offsetOfLastElementInblock = *(len_t*)(kv->bh.block + getOffsetBlock(nbSlots - 1));
        for (unsigned int i = 0; i < nbSlots; i++) {
            if (offsetKV == *(len_t*)(kv->bh.block + getOffsetBlock(i))) {  // found element
                // swaps the offset of the element with the one of the last element of the bloc
                memcpy(kv->bh.block + getOffsetBlock(i), &offsetOfLastElementInblock, sizeof(unsigned int));
                nbSlots--;
                setNbElementsInBlock(kv->bh.block, nbSlots);

                if (nbSlots == 0 && previousblock == 0 &&
                    hasNextBlock(kv->bh.block) == false) {  // frees the blockentry
                    if (freeHashtableEntry(kv, hash) == -1)
                        return -1;
                    kv->bh.blockIsOccupied[blockIndex] = false;  // marks the blockas free
                    printf("marks block %d as free\n", blockIndex);
                }
                return 1;
            }
        }
        // entry not found
        previousblock = blockIndex;
        blockIndex = getIndexNextBlock(kv->bh.block);  // index of next block
        if (readNewBlock(blockIndex, kv) == -1)
            return 0;
    } while (true);
    return 1;
}

// Sets hashtable entry to -1, i.e. sets the entry to free
int freeHashtableEntry(KV* kv, int hash) {
    return writeBlockIndexToH(kv, hash, -1);
}

void kv_start(KV* kv) {
    if (strcmp(kv->fds.mode, "w") == 0) {  // no rights to read
        errno = EACCES;
    }
    lseek(kv->fds.fd_kv, LG_EN_TETE_KV, SEEK_SET);
    kv->dkvh.nextTuple = 0;  // initializes the number of read couples to 0
}

int kv_next(KV* kv, kv_datum* key, kv_datum* val) {
    if (strcmp(kv->fds.mode, "w") == 0) {
        errno = EACCES;
        return -1;
    }
    while (kv->dkvh.nextTuple < getSlotsInDKV(kv) && getLengthDkv(kv, kv->dkvh.nextTuple) > 0)
        kv->dkvh.nextTuple++;  // jumps empty slots

    if (getSlotsInDKV(kv) == kv->dkvh.nextTuple)
        return 0;

    len_t tupleOffset = access_offset_dkv(kv->dkvh.nextTuple, kv);
    if (fillsKey(kv, tupleOffset, key) == -1)
        return -1;
    if (fillValue(kv, tupleOffset, val, key) == -1)
        return -1;
    kv->dkvh.nextTuple++;
    return 1;
}

// Reads the key stored at position offset into key
// Returns 1 on success and -1 on failure
int fillsKey(KV* kv, len_t offsetKV, kv_datum* key) {
    if (key == NULL) {  // key must have been allocated
        errno = EINVAL;
        return -1;
    }

    key->len = getKeyLengthFromKV(kv, offsetKV);
    if (key->len == 0)
        return -1;
    key->ptr = malloc(key->len);
    if (readAtPosition(kv->fds.fd_kv, offsetKV + sizeof(len_t), key->ptr, key->len, kv) == -1)
        return -1;
    return 1;
}

// Returns size of key or -1 when error
len_t getKeyLengthFromKV(KV* kv, len_t offsetKV) {
    len_t length;
    if (readAtPosition(kv->fds.fd_kv, offsetKV, &length, sizeof(len_t), kv) == -1)
        return 0;
    return length;
}

// Truncates KV
int truncate_kv(KV* kv) {
    if (strcmp(kv->fds.mode, "r") == 0)  // don't truncate since kv wasn't changed
        return 1;
    len_t maxOffset = 0;
    len_t maxLength = 0;
    unsigned int nbElementsInDKV = getSlotsInDKV(kv);
    for (unsigned int i = 0; i < nbElementsInDKV; i++) {
        if (maxOffset <= access_offset_dkv(i, kv)) {
            maxOffset = access_offset_dkv(i, kv);
            maxLength = abs(access_lg_dkv(i, kv));
        }
    }
    if (ftruncate(kv->fds.fd_kv, maxOffset + maxLength + LG_EN_TETE_KV) == -1)
        return -1;
    return 1;
}
