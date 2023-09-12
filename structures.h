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
 * Représente une donnée : soit une key, soit une valeur
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

#define LG_EN_TETE_H 5     // MagicN + hashFunctionIndex
#define LG_EN_TETE_BLK 5   // MagicN + nb_blocs
#define LG_EN_TETE_KV 1    // MagicN
#define LG_EN_TETE_DKV 5   // MagicN + nb_blocs
#define LG_EN_TETE_BLOC 9  // indexNextBlocWithSameHash(char) + indexNextBloc(int) + numberOfSlots(int)

#define BLOCK_SIZE 4096

struct s_KV {
    int fd_h;                  // File descriptor of h file
    int fd_blk;                // File descriptor of blk file
    int fd_kv;                 // File descriptor of kv file
    int fd_dkv;                // File descriptor of dkv file
    alloc_t allocationMethod;  // FIRST_FIT, WORST_FIT or BEST_FIT
    unsigned char* dkv;        // Holds dkv
    unsigned int maxElementsInDKV;
    const char* mode;
    unsigned int nextTuple;
    int (*hashFunction)(const kv_datum* kdatum);
    unsigned char bloc[BLOCK_SIZE];
    bool* blocIsOccupied;  // array containing whether blocs are occupied or not. blocIsOccupied[i]=true means that bloc
                           // i is occupied
    unsigned int blocIsOccupiedSize;
    unsigned int nb_blocs;
};
typedef struct s_KV KV;

#endif