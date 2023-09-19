#ifndef COMMON_FUNCTIONS_H
#define COMMON_FUNCTIONS_H

#include "structures.h"

char* concat(const char* S1, const char* S2);

int read_controle(int descripteur, void* ptr, int nboctets);
int write_controle(int descripteur, const void* ptr, int nboctets);

int readAtPosition(int fd, unsigned int position, void* dest, unsigned int nbBytes, KV* database);
int writeAtPosition(int fd, unsigned int position, void* src, unsigned int nbBytes, KV* database);

int readNewBlockBLK(unsigned int index, KV* database);

int closeFileDescriptors(KV* database);

int getNbElementsInBlockBLK(unsigned int index, KV* kv);
int getIndexNextBlockBLK(unsigned int index, KV* kv);
int setOffsetInBLK(unsigned int offsetKV, unsigned int blockIndex, KV* kv, unsigned int slot);
int setNbElementsInBlockBLK(unsigned int nbElements, unsigned int blockIndex, KV* kv);
int getOffsetKVBlockBLK(unsigned int indexInBlock, unsigned int blockIndex, KV* kv);
bool hasNextBlockBLK(unsigned int blockIndex, KV* kv);
int getNextBlockBLK(unsigned int blockIndex, KV* kv);

// Return offsets
static inline len_t getOffsetBlk(unsigned int index) {
    return (index - 1) * BLOCK_SIZE + LG_EN_TETE_BLK;
}

static inline len_t getOffsetH(unsigned int hash) {
    return LG_EN_TETE_H + hash * sizeof(unsigned int);
}

static inline len_t getOffsetBlock(unsigned int index) {
    return LG_EN_TETE_BLOCK + index * sizeof(unsigned int);
}

static inline len_t getOffsetDkv(unsigned int index) {
    return LG_EN_TETE_DKV + index * 2 * sizeof(unsigned int);
}

static inline int getLengthDkv(KV* kv, unsigned int index) {  // signed int because *-1 to tag slot as taken!
    return *((int*)(kv->dkvh.dkv + getOffsetDkv(index)));
}

static inline int getSlotSizeDkv(KV* kv, unsigned int index) {
    return *((int*)(kv->dkvh.dkv + getOffsetDkv(index)));  // keeps as int since < 0 means that the slot is not free
}

static inline unsigned int getSlotOffsetDkv(KV* kv, unsigned int index) {
    return *((int*)(kv->dkvh.dkv + getOffsetDkv(index) + sizeof(unsigned int)));
}

static inline void setSlotSizeDkv(KV* kv, unsigned int index, unsigned int size) {
    memcpy(kv->dkvh.dkv + getOffsetDkv(index), &size, sizeof(unsigned int));
}

static inline void setSlotOffsetDkv(KV* kv, unsigned int index, unsigned int size) {
    memcpy(kv->dkvh.dkv + getOffsetDkv(index) + sizeof(unsigned int), &size, sizeof(unsigned int));
}

static inline len_t getOffsetKV(unsigned int index) {
    return LG_EN_TETE_KV + index;
}

static inline unsigned int getSlotsInDKV(KV* kv) {
    return *(int*)(kv->dkvh.dkv + 1);
}

static inline void setSlotsInDKV(KV* kv, unsigned int nbSlots) {
    memcpy(kv->dkvh.dkv + 1, &nbSlots, sizeof(unsigned int));
}

static inline unsigned int sizeOfDKVFilled(KV* kv) {
    return getSlotsInDKV(kv) * (sizeof(unsigned int) + sizeof(len_t)) + LG_EN_TETE_DKV;
}

static inline unsigned int sizeOfDKVMax(KV* kv) {
    return kv->dkvh.maxElementsInDKV * (sizeof(unsigned int) + sizeof(len_t)) + LG_EN_TETE_DKV;
}

#endif