#ifndef COMMON_FUNCTIONS_H
#define COMMON_FUNCTIONS_H

#include "structures.h"

char* concat(const char* S1, const char* S2);

int read_controle(int descripteur, void* ptr, int nboctets);
int write_controle(int descripteur, const void* ptr, int nboctets);

int readAtPosition(int fd, unsigned int position, void* dest, unsigned int nbBytes, KV* database);
int writeAtPosition(int fd, unsigned int position, void* src, unsigned int nbBytes, KV* database);

int closeFileDescriptors(KV* database);

// Frees the memory allocated by KV
void freeDatabase(KV* database);

// Return offsets

static inline len_t getOffsetDkv(unsigned int index) {
    return HEADER_SIZE_DKV + index * 2 * sizeof(unsigned int);
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
    return HEADER_SIZE_KV + index;
}

static inline unsigned int getSlotsInDKV(KV* kv) {
    return *(int*)(kv->dkvh.dkv + 1);
}

static inline void setSlotsInDKV(KV* kv, unsigned int nbSlots) {
    memcpy(kv->dkvh.dkv + 1, &nbSlots, sizeof(unsigned int));
}

static inline unsigned int sizeOfDKVFilled(KV* kv) {
    return getSlotsInDKV(kv) * (sizeof(unsigned int) + sizeof(len_t)) + HEADER_SIZE_DKV;
}

static inline unsigned int sizeOfDKVMax(KV* kv) {
    return kv->dkvh.maxElementsInDKV * (sizeof(unsigned int) + sizeof(len_t)) + HEADER_SIZE_DKV;
}

#endif