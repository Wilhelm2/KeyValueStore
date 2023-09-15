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

#define LG_EN_TETE_H 5      // MagicN + hashFunctionIndex
#define LG_EN_TETE_BLK 5    // MagicN + nb_blocks
#define LG_EN_TETE_KV 1     // MagicN
#define LG_EN_TETE_DKV 5    // MagicN + nb_blocks
#define LG_EN_TETE_BLOCK 9  // hasNextBlock(bool) + indexNextBlockWithSameHash(int) + numberOfSlots(int)

#define BLOCK_SIZE 4096

typedef struct {
    int fd_h;    // File descriptor of h file
    int fd_blk;  // File descriptor of blk file
    int fd_kv;   // File descriptor of kv file
    int fd_dkv;  // File descriptor of dkv file
    const char* mode;
} fileDescriptors;

typedef struct {
    unsigned char block[BLOCK_SIZE];
    unsigned int indexCurrLoadedBlock;
    unsigned int blockIsOccupiedSize;
    bool* blockIsOccupied;  // array containing whether blocks are occupied or not. blockIsOccupied[i]=true means that
                            // block i is occupied
    unsigned int nb_blocks;
} blockHandler;

typedef struct {
    unsigned char* dkv;  // Holds dkv
    unsigned int maxElementsInDKV;
    unsigned int nextTuple;
} dkvHandler;

typedef struct {
    fileDescriptors fds;

    blockHandler bh;

    dkvHandler dkvh;

    alloc_t allocationMethod;  // FIRST_FIT, WORST_FIT or BEST_FIT
    unsigned int (*hashFunction)(const kv_datum* kdatum);
} KV;

#endif