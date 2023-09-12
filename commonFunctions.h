#ifndef COMMON_FUNCTIONS_H
#define COMMON_FUNCTIONS_H

typedef struct s_KV KV;

#include "structures.h"

char* concat(const char* S1, const char* S2);

int read_controle(int descripteur, void* ptr, int nboctets);
int write_controle(int descripteur, const void* ptr, int nboctets);

int readAtPosition(int fd, unsigned int position, void* dest, unsigned int nbBytes, KV* database);
int writeAtPosition(int fd, unsigned int position, void* src, unsigned int nbBytes, KV* database);
int closeFileDescriptors(KV* database);

// Return offsets
static inline len_t getOffsetBlk(unsigned int index) {
    return (index - 1) * BLOCK_SIZE + LG_EN_TETE_BLK;
}

static inline len_t getOffsetH(unsigned int hash) {
    return LG_EN_TETE_H + hash * sizeof(unsigned int);
}

static inline len_t getOffsetBloc(unsigned int index) {
    return LG_EN_TETE_BLOC + index * sizeof(unsigned int);
}

static inline len_t getOffsetDkv(unsigned int index) {
    return LG_EN_TETE_DKV + index * 2 * sizeof(unsigned int);
}

static inline int getLengthDkv(KV* kv, unsigned int index) {
    return *((int*)(kv->dkv + getOffsetDkv(index)));
}

static inline int getSlotSizeDkv(KV* kv, unsigned int index) {
    return *((int*)(kv->dkv + getOffsetDkv(index)));  // keeps as int since < 0 means that the slot is not free
}

static inline len_t getOffsetKV(unsigned int index) {
    return LG_EN_TETE_KV + index;
}

static inline unsigned int getSlotsInDKV(KV* kv) {
    return *(int*)(kv->dkv + 1);
}

static inline void setSlotsInDKV(KV* kv, unsigned int nbSlots) {
    memcpy(kv->dkv + 1, &nbSlots, sizeof(unsigned int));
}

static inline unsigned int getNbElementsInBlock(unsigned char* bloc) {
    return *(int*)(bloc + 5);
}

static inline unsigned int sizeOfDKVFilled(KV* kv) {
    return getSlotsInDKV(kv) * (sizeof(unsigned int) + sizeof(len_t)) + LG_EN_TETE_DKV;
}

static inline unsigned int sizeOfDKVMax(KV* kv) {
    return kv->maxElementsInDKV * (sizeof(unsigned int) + sizeof(len_t)) + LG_EN_TETE_DKV;
}

#endif