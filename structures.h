#ifndef STRUCTURES_H
#define STRUCTURES_H

#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

// 32 bits allow a database size of up to 4 Go
typedef uint32_t len_t;

/*
 * Représente une donnée : soit une clef, soit une valeur
 */

struct kv_datum {
    void* ptr; /* la donnée elle-même */
    len_t len; /* sa taille */
};

typedef struct kv_datum kv_datum;

/*
 * Les différents types d'allocation
 */

typedef enum { FIRST_FIT, WORST_FIT, BEST_FIT } alloc_t;

#define LG_EN_TETE_H 5     // MagicN + index_f_hache
#define LG_EN_TETE_BLK 5   // MagicN + nb_bloc
#define LG_EN_TETE_KV 1    // MagicN
#define LG_EN_TETE_DKV 5   // MagicN + nb_blocs
#define LG_EN_TETE_BLOC 9  // bloc suivant(char)+nr bloc suivant(int) + nb emplacements (int)

struct s_KV {
    int descr_h;
    int descr_blk;
    int descr_kv;
    int descr_dkv;
    alloc_t methode_alloc;
    unsigned char* dkv;
    len_t remplissement_dkv;
    len_t longueur_dkv;
    const char* mode;
    int couple_nr_kv_next;
    int (*f_hachage)(const kv_datum* kdatum);
    unsigned char* bloc;
    int key_long;
    int* tabbloc;
    unsigned int longueur_buf_bloc;
    int nb_blocs;
};
typedef struct s_KV KV;

#endif