#ifndef KV_H
#define KV_H

#include "hashFunctions.h"
#include "kvInitialization.h"
#include "slotAllocations.h"
#include "structures.h"

KV* kv_open(const char* dbname, const char* mode, int hidx, alloc_t alloc);
int closeFileDescriptors(KV* database);
int kv_close(KV* kv);
void freeDatabase(KV* database);
int kv_get(KV* kv, const kv_datum* key, kv_datum* val);
int kv_put(KV* kv, const kv_datum* key, const kv_datum* val);
int writeElementToKV(KV* kv, const kv_datum* key, const kv_datum* val, len_t offset);

int kv_del(KV* kv, const kv_datum* key);
void kv_start(KV* kv);
int kv_next(KV* kv, kv_datum* key, kv_datum* val);

// Accès à dkv
len_t access_offset_dkv(int emplacement, KV* kv);
unsigned int access_lg_dkv(unsigned int emplacement, KV* kv);

// fonctions utilisées dans l'ordre :
int readsBlockOccupiedness(KV* kv);
int RechercheBlocH(KV* kv, int hash);
len_t lookupKeyOffset(KV* kv, const kv_datum* key, int numbloc);
int compareKeys(KV* kv, const kv_datum* key, len_t offset);
len_t RechercheTaillekey(KV* kv, len_t offset);
int fillValue(KV* kv, len_t offset, kv_datum* val, const kv_datum* key);
int fillsKey(KV* kv, len_t offset, kv_datum* key);
int AllocatesNewBlock(KV* kv);
int writeBlockIndexToH(KV* kv, int hash, int numbloc);
void insertionFusionEspace(KV* kv, unsigned int emplacement_dkv);
void fusionVoisinsVidesSP(int voisins, int emplacement_dkv, int voisinp, KV* kv);
void decaledkv_arriere(KV* kv, int slotToDelete, int centralSlot);
void libereEmplacementdkv(len_t offset, KV* kv);
int libereEmplacementblk(int numbloc, len_t offset, KV* kv, int hash, int previousBloc);
int supprimeblocdeh(KV* kv, int hash);
void createNewSlotEndDKV(KV* kv, const kv_datum* key, const kv_datum* val);
KV* increaseSizeDkv(KV* kv);
int lienBlkKv(int numbloc, KV* kv, len_t emplacement_kv);
KV* SetSlotDKVAsOccupied(KV* kv, int emplacement_dkv);
void insertionNewEspace(KV* kv, int emp_dkv, const kv_datum* key, const kv_datum* val);
void creationNewVoisin(KV* kv, int octets_restants, int empl_dkv, len_t requiredSize);
void decaledkv_avant(KV* kv, int emplacement_a_decal);
int truncate_kv(KV* kv);

/*
 * Définition de l'API de la bibliothèque kv
 */

KV* kv_open(const char* dbname, const char* mode, int hidx, alloc_t alloc);
int kv_close(KV* kv);
int kv_get(KV* kv, const kv_datum* key, kv_datum* val);
int kv_put(KV* kv, const kv_datum* key, const kv_datum* val);
int kv_del(KV* kv, const kv_datum* key);
void kv_start(KV* kv);
int kv_next(KV* kv, kv_datum* key, kv_datum* val);
unsigned int getNbElementsInBlock(unsigned char* bloc);

#endif