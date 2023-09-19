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

    if (writeElementToKV(kv, key, val, getSlotOffsetDkv(kv, dkvSlot)) == -1)  // Writes tuple to KV
        return -1;
    if (addsEntryToBLK(kv, getSlotOffsetDkv(kv, dkvSlot), blockIndex) == -1)
        return -1;
    printf("adds element to blk blockindex %d dkvslot %d offset %d\n", blockIndex, dkvSlot,
           getSlotOffsetDkv(kv, dkvSlot));
    return 0;
}

int getBlockIndexForHash(unsigned int hash, KV* kv) {
    int blockIndex;
    // First looks the block up. Returns -1 when the hash is associated to no block with errno set to EINVAL
    if ((blockIndex = getBlockIndexWithFreeEntry(kv, hash)) == -1 && errno == EINVAL) {
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
    bool hasNextBlock;

    // Reads the block index contained at the hash position
    if ((test = readAtPosition(kv->fds.fd_h, getOffsetH(hash), &blockIndex, sizeof(int), kv)) == -1)
        return -1;
    if (test == 0 || blockIndex == -1) {  // either read 0 bytes or has no registered block
        errno = EINVAL;
        return -1;
    }
    printf("read blockindex %d elements in blk %d test %d hash %d\n", blockIndex,
           getNbElementsInBlockBLK(blockIndex, kv), test, hash);
    do {
        int nbElements = getNbElementsInBlockBLK(blockIndex, kv);
        if (nbElements == -1)
            return -1;
        if ((nbElements + 1) * sizeof(len_t) < BLOCK_SIZE - LG_EN_TETE_BLOCK)
            return blockIndex;  // Space for another slot
        hasNextBlock = hasNextBlockBLK(blockIndex, kv);
        blockIndex = getIndexNextBlockBLK(blockIndex, kv);
        if (blockIndex == -1)
            return -1;
    } while (hasNextBlock);

    errno = EINVAL;
    return -1;
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
    printf("allocates new slot at end of dkv\n");
    if (getSlotsInDKV(kv) == kv->dkvh.maxElementsInDKV)
        increaseSizeDkv(kv);

    len_t lengthNewSlot = sizeof(len_t) + key->len + sizeof(len_t) + val->len;
    memcpy(kv->dkvh.dkv + getOffsetDkv(getSlotsInDKV(kv)), &lengthNewSlot, sizeof(len_t));
    if (getSlotsInDKV(kv) == 0)
        offsetNewSlot = LG_EN_TETE_KV;
    else
        offsetNewSlot = getSlotOffsetDkv(kv, getSlotsInDKV(kv) - 1) + abs(getSlotSizeDkv(kv, getSlotsInDKV(kv) - 1));
    memcpy(kv->dkvh.dkv + getOffsetDkv(getSlotsInDKV(kv)) + sizeof(len_t), &offsetNewSlot, sizeof(len_t));
    printf("writes length %d of new slot at offset %d\n", lengthNewSlot, getOffsetDkv(getSlotsInDKV(kv)));
    printf("writes offset %d of new slot at offset %d\n", offsetNewSlot, getSlotOffsetDkv(kv, getSlotsInDKV(kv)));
    printf("writes offset %d of new slot at dkvoffset %d\n", offsetNewSlot, getOffsetDkv(getSlotsInDKV(kv)));
    if (getSlotsInDKV(kv) > 0) {
        printf("previous slot offset %d length %d\n", getSlotOffsetDkv(kv, getSlotsInDKV(kv) - 1),
               abs(getSlotSizeDkv(kv, getSlotsInDKV(kv) - 1)));
    }
    setSlotsInDKV(kv, getSlotsInDKV(kv) + 1);
    printf("incr nbslots to %d in createnewslotendkv\n", getSlotsInDKV(kv));
}

// Increases the size of dkv
void increaseSizeDkv(KV* database) {
    database->dkvh.maxElementsInDKV += 1000;
    database->dkvh.dkv = realloc(database->dkvh.dkv, sizeOfDKVMax(database));
}

// Sets DKV slot as occupied by multiplying its length by -1
void SetSlotDKVAsOccupied(KV* kv, int dkvSlot) {
    int length = getSlotSizeDkv(kv, dkvSlot) * (-1);
    memcpy(kv->dkvh.dkv + getOffsetDkv(dkvSlot), &length, sizeof(int));
}

// Splits the DKV slot in two if there is enough space to contain another slot
void trySplitDKVSlot(KV* kv, unsigned int slotToSplit, unsigned int slotToSplitRequiredLength) {
    len_t remainingBytes = (-1) * getSlotSizeDkv(kv, slotToSplit) - slotToSplitRequiredLength;
    if (remainingBytes > (2 * sizeof(unsigned int) + 2)) {
        printf("splits the slot %d\n", slotToSplit);
        // Size for another slot of at least one character for key and value
        insertNewSlotDKV(kv, slotToSplit);
        setSlotSizeDkv(kv, slotToSplit, slotToSplitRequiredLength * (-1));  //*-1 to indicate that slot is taken
        setSlotSizeDkv(kv, slotToSplit + 1, remainingBytes);
        setSlotOffsetDkv(kv, slotToSplit + 1, getSlotOffsetDkv(kv, slotToSplit) + slotToSplitRequiredLength);
    }
}

// Shifts slots one to the right
void insertNewSlotDKV(KV* database, int firstSlot) {
    if (getSlotsInDKV(database) == database->dkvh.maxElementsInDKV)  // adds space to dkv
        increaseSizeDkv(database);

    memcpy(database->dkvh.dkv + getOffsetDkv(firstSlot + 1), database->dkvh.dkv + getOffsetDkv(firstSlot),
           sizeOfDKVFilled(database) - getOffsetDkv(firstSlot));
    setSlotsInDKV(database, getSlotsInDKV(database) + 1);
    printf("incr nbslots to %d in insertnewslotdkv\n", getSlotsInDKV(database));
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
    printf("write key length %d key %.*s offset %d\n", key->len, key->len, (char*)key->ptr, offsetKV);

    if (writeAtPosition(kv->fds.fd_kv, offsetKV, tabwrite, nbBytesToWrite, kv) == -1)
        return -1;

    free(tabwrite);
    return 1;
}

// Writes KV offset to BLK
int addsEntryToBLK(KV* kv, len_t offsetKV, unsigned int blockIndex) {
    int nbSlots = getNbElementsInBlockBLK(blockIndex, kv);
    if (nbSlots == -1)
        return -1;
    if (setOffsetInBLK(offsetKV, blockIndex, kv, nbSlots) == -1)
        return -1;
    if (setNbElementsInBlockBLK(nbSlots + 1, blockIndex, kv) == -1)
        return -1;
    return 1;
}

// Lookup the value associated to key
// Returns 1 when found, 0 when not found, and -1 when error
int kv_get(KV* kv, const kv_datum* key, kv_datum* val) {
    unsigned int hash = kv->hashFunction(key);
    int blockIndex, readTest;
    int offset;
    if (key == NULL || key->ptr == NULL || val == NULL || key->len < 1) {
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

    // Lookup the key in KV. Returns offset when found
    offset = lookupKeyOffsetKV(kv, key, blockIndex);
    printf("hash %d blockIndex %d readTest %d offset %d\n", hash, blockIndex, readTest, offset);
    if (offset == 0)
        return 0;
    if (fillValue(kv, offset, val, key) == -1)
        return -1;
    return 1;
}

// Lookup the offset of key key
// Returns offset when found, -1 when error, and 0 when not found
len_t lookupKeyOffsetKV(KV* kv, const kv_datum* key, int blockIndex) {
    int test;
    do {
        for (int i = 0; i < getNbElementsInBlockBLK(blockIndex, kv); i++) {
            test = compareKeys(kv, key, getOffsetKVBlockBLK(i, blockIndex, kv));
            // printf("checks element %d cmp result %d nbelementsinblock %d\n", i, test,
            //        getNbElementsInBlockBLK(blockIndex, kv));
            if (test == -1)
                return 0;
            else if (test == 1)
                return getOffsetKVBlockBLK(i, blockIndex, kv);
        }
        if (hasNextBlockBLK(blockIndex, kv)) {
            blockIndex = getNextBlockBLK(blockIndex, kv);
            if (readNewBlockBLK(getIndexNextBlockBLK(blockIndex, kv), kv) == -1)
                return 0;
        } else
            blockIndex = -1;
    } while (blockIndex != -1);

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
    //    printf("key->len %d keyLength %d offset %d\n", key->len, keyLength, offsetKV);

    if (keyLength != key->len)
        return 0;
    while (totalReadBytes < keyLength) {
        maxReadBytesInStep = (keyLength - totalReadBytes) % 2049;  // reads at most 2048 bytes at each step
        readBytes = read_controle(kv->fds.fd_kv, buf, maxReadBytesInStep);
        if (readBytes == -1)
            return -1;
        //      printf("length %d key %.*s offset %d\n", readBytes, readBytes, buf, offsetKV);

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

    offset = lookupKeyOffsetKV(kv, key, blockIndex);
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
        if (getSlotOffsetDkv(kv, i) == offsetKV) {
            int length = (-1) * getSlotSizeDkv(kv, i);  // multiplies by -1 to tag the slot as free
            printf("frees the dkv slot %d offset %d length %d\n", i, getOffsetDkv(i), length);
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
        if (getSlotSizeDkv(database, centralSlot - 1) > 0) {  // free slot -> merge
            mergeSlots(centralSlot - 1, centralSlot, database);
            centralSlot--;  // since just removed the left slot
        }
    }
    if (centralSlot < getSlotsInDKV(database) - 1) {
        if (getSlotSizeDkv(database, centralSlot + 1) > 0)  // free slot -> merge
            mergeSlots(centralSlot, centralSlot + 1, database);
    }
}

void mergeSlots(unsigned int left, unsigned int right, KV* database) {
    printf("merge slots %d and %d", left, right);
    printf("offset %d and %d\n", getSlotOffsetDkv(database, left), getSlotOffsetDkv(database, right));
    printf("length %d and %d\n", getSlotSizeDkv(database, left), getSlotSizeDkv(database, right));
    len_t newSize = getSlotSizeDkv(database, left) + getSlotSizeDkv(database, right);
    len_t offset = getSlotOffsetDkv(database, left);
    memcpy(database->dkvh.dkv + getOffsetDkv(left), database->dkvh.dkv + getOffsetDkv(right),
           sizeOfDKVFilled(database) - getOffsetDkv(right));
    memcpy(database->dkvh.dkv + getOffsetDkv(left), &newSize, sizeof(len_t));
    memcpy(database->dkvh.dkv + getOffsetDkv(left) + sizeof(int), &offset, sizeof(len_t));
    setSlotsInDKV(database, getSlotsInDKV(database) - 1);
}

// Frees an entry in blk. Returns 1 on success and -1 on failure
int freeEntryBLK(int blockIndex, int offsetKV, KV* kv, int hash, int previousblock) {
    do {
        int nbSlots = getNbElementsInBlockBLK(blockIndex, kv);
        if (nbSlots == -1)
            return -1;
        int offsetOfLastElementInblock = getOffsetKVBlockBLK(nbSlots - 1, blockIndex, kv);
        if (offsetOfLastElementInblock == -1)
            return -1;
        for (int i = 0; i < nbSlots; i++) {
            if (offsetKV == getOffsetKVBlockBLK(i, blockIndex, kv)) {  // found element
                // swaps the offset of the element with the one of the last element of the bloc
                setOffsetInBLK(offsetOfLastElementInblock, blockIndex, kv, i);
                nbSlots--;
                setNbElementsInBlockBLK(nbSlots, blockIndex, kv);

                if (nbSlots == 0 && previousblock == 0 &&
                    hasNextBlockBLK(blockIndex, kv) == false) {  // frees the blockentry
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
        blockIndex = getIndexNextBlockBLK(blockIndex, kv);  // index of next block
        if (readNewBlockBLK(blockIndex, kv) == -1)
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

    len_t tupleOffset = getSlotOffsetDkv(kv, kv->dkvh.nextTuple);
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
        if (maxOffset <= getSlotOffsetDkv(kv, i)) {
            maxOffset = getSlotOffsetDkv(kv, i);
            maxLength = abs(getSlotSizeDkv(kv, i));
        }
    }
    if (ftruncate(kv->fds.fd_kv, maxOffset + maxLength + LG_EN_TETE_KV) == -1)
        return -1;
    return 1;
}

void verifyEntriesDKV(KV* database) {
    bool error = false;
    for (unsigned int i = 0; i < getSlotsInDKV(database); i++) {
        //        printf("slot %d offset %d length %d", i, getSlotOffsetDkv(database, i), getSlotSizeDkv(database, i));
        if (i > 0) {
            //  printf("no lost space %d", (getSlotOffsetDkv(database, i) - (getSlotOffsetDkv(database, i - 1) +
            //                                                             abs(getSlotSizeDkv(database, i - 1)))) == 0);
            if (!(getSlotOffsetDkv(database, i) -
                  (getSlotOffsetDkv(database, i - 1) + abs(getSlotSizeDkv(database, i - 1)))) == 0) {
                printf("error at entry %d\n", i);
                printf("\noffsets %d %d lengths %d %d", getSlotOffsetDkv(database, i - 1),
                       getSlotOffsetDkv(database, i), getSlotSizeDkv(database, i - 1), getSlotSizeDkv(database, i));
                printf("\n");
                kv_datum key;
                fillsKey(database, getSlotOffsetDkv(database, i), &key);
                printf(" key length %d key %.*s offset %d\n", key.len, key.len, (char*)key.ptr,
                       getSlotOffsetDkv(database, i));
                error = true;
            }
            // printf("\n");
        }
    }
    if (error)
        printf("ERROR IN DKV\n");
    else
        printf("NO ERROR DETECTED IN DKV\n");
}