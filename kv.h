#include "slotAllocations.h"
#include "structures.h"

KV* kv_open(const char* dbname, const char* mode, int hidx, alloc_t alloc);
int initializeFileDescriptors(KV* database, const char* mode, const char* dbname);
int readAtPosition(int fd, unsigned int position, void* dest, unsigned int nbBytes, KV* database);
int writeAtPosition(int fd, unsigned int position, void* src, unsigned int nbBytes, KV* database);
int getOpenMode(const char* mode);
int closeFileDescriptors(KV* database);
int tryOpen(KV* database, const char* filename, int flags, unsigned int mode);
int kv_close(KV* kv);
void freeDatabase(KV* database);
int kv_get(KV* kv, const kv_datum* key, kv_datum* val);
int kv_put(KV* kv, const kv_datum* key, const kv_datum* val);
int kv_del(KV* kv, const kv_datum* key);
void kv_start(KV* kv);
int kv_next(KV* kv, kv_datum* key, kv_datum* val);

// Fonctions générales
int read_controle(int descripteur, void* ptr, int nboctets);
int write_controle(int descripteur, const void* ptr, int nboctets);

// Fonctions de hachage
int (*choix_hachage(int index))(const kv_datum* clef);
int hachage_defaut(const kv_datum* clef);
int hachage_test(const kv_datum* clef);
int djb_hash(const kv_datum* clef);
int fnv_hash(const kv_datum* clef);

// Modification
unsigned char* modif(unsigned char* buf, int i, int debut);
int absolue(int val);

// Accès à dkv
len_t access_offset_dkv(int emplacement, KV* kv);
unsigned int access_lg_dkv(unsigned int emplacement, KV* kv);

// fonctions utilisées dans l'ordre :
char* concat(const char* s1, const char* s2);
int verificationMagicN(int descr, const char* mode, unsigned char magicNumber, int test);
int remplit_bloc(int debut, KV* kv);
int RechercheBlocH(KV* kv, int hache);
len_t RechercheOffsetClef(KV* kv, const kv_datum* key, int numbloc);
int compareClefkv(KV* kv, const kv_datum* key, len_t offset);
len_t RechercheTailleClef(KV* kv, len_t offset);
int RemplissageVal(KV* kv, len_t offset, kv_datum* val, const kv_datum* key);
int RemplissageClef(KV* kv, len_t offset, kv_datum* clef);
int Allouebloc(KV* kv);
int liaisonHBlk(KV* kv, int hache, int numbloc);
void insertionFusionEspace(KV* kv, int emplacement_dkv);
void fusionVoisinsVidesSP(int voisins, int emplacement_dkv, int voisinp, KV* kv);
void decaledkv_arriere(KV* kv, int emplacement_a_sup);
void libereEmplacementdkv(len_t offset, KV* kv);
int libereEmplacementblk(int numbloc, len_t offset, KV* kv, int hache, int bloc_p);
int supprimeblocdeh(KV* kv, int hache);
void NouvEmplacementDkv(KV* kv, const kv_datum* key, const kv_datum* val);
KV* dkv_aggrandissement(KV* kv);
unsigned char* copie_tableau(unsigned char* tableau, unsigned int longueur);
int lienBlkKv(int numbloc, KV* kv, len_t emplacement_kv);
KV* SetOccupeDkv(KV* kv, int emplacement_dkv);
void insertionNewEspace(KV* kv, int emp_dkv, const kv_datum* key, const kv_datum* val);
void creationNewVoisin(KV* kv, int octets_restants, int empl_dkv, len_t t_couple);
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