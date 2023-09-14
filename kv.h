#ifndef KV_H
#define KV_H

#include "hashFunctions.h"
#include "kvInitialization.h"
#include "slotAllocations.h"
#include "structures.h"

KV* kv_open(const char* dbname, const char* mode, int hashFunctionIndex, alloc_t alloc);
int closeFileDescriptors(KV* database);
int kv_close(KV* kv);
void freeDatabase(KV* database);
int kv_get(KV* kv, const kv_datum* key, kv_datum* val);
int kv_put(KV* kv, const kv_datum* key, const kv_datum* val);
int allocatesDkvSlot(KV* database, const kv_datum* key, const kv_datum* val);
int getBlockIndexForHash(unsigned int hash, KV* kv);
void trySplitDKVSlot(KV* kv, unsigned int slotToSplit, unsigned int slotToSplitRequiredLength);
void SetSlotDKVAsOccupied(KV* kv, int emplacement_dkv);
int writeElementToKV(KV* kv, const kv_datum* key, const kv_datum* val, len_t offset);

int kv_del(KV* kv, const kv_datum* key);
void kv_start(KV* kv);
int kv_next(KV* kv, kv_datum* key, kv_datum* val);

// Accès à dkv
len_t access_offset_dkv(int emplacement, KV* kv);
unsigned int access_lg_dkv(unsigned int emplacement, KV* kv);

// fonctions utilisées dans l'ordre :
int getBlockIndexWithFreeEntry(KV* kv, int hash);
len_t lookupKeyOffset(KV* kv, const kv_datum* key, int numbloc);
int compareKeys(KV* kv, const kv_datum* key, len_t offset);
len_t RechercheTaillekey(KV* kv, len_t offset);
int fillValue(KV* kv, len_t offset, kv_datum* val, const kv_datum* key);
int fillsKey(KV* kv, len_t offset, kv_datum* key);
int AllocatesNewBlock(KV* kv);
int writeBlockIndexToH(KV* kv, int hash, int numbloc);
void libereEmplacementdkv(len_t offset, KV* kv);
void tryMergeSlots(KV* database, unsigned int centralSlot);
void mergeSlots(unsigned int left, unsigned int right, KV* database);

int libereEmplacementblk(int numbloc, len_t offset, KV* kv, int hash, int previousBloc);
int supprimeBlockdeh(KV* kv, int hash);
void createNewSlotEndDKV(KV* kv, const kv_datum* key, const kv_datum* val);
void increaseSizeDkv(KV* kv);
void writeOffsetToBLK(KV* kv, len_t emplacement_kv);
void creationNewVoisin(KV* kv, int octets_restants, int empl_dkv, len_t requiredSize);
void insertNewSlotDKV(KV* kv, int emplacement_a_decal);
int truncate_kv(KV* kv);

/*
 * Définition de l'API de la bibliothèque kv
 */

KV* kv_open(const char* dbname, const char* mode, int hashFunctionIndex, alloc_t alloc);
int kv_close(KV* kv);
int kv_get(KV* kv, const kv_datum* key, kv_datum* val);
int kv_put(KV* kv, const kv_datum* key, const kv_datum* val);
int kv_del(KV* kv, const kv_datum* key);
void kv_start(KV* kv);
int kv_next(KV* kv, kv_datum* key, kv_datum* val);
unsigned int getNbElementsInBlock(unsigned char* bloc);

int readNewBlock(unsigned int index, KV* database);
#endif