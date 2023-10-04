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

// 32 bits allow a database size of up to 4 Go.
typedef uint32_t len_t;

// Contains a value and its length. Data type used for keys and values.
struct kv_datum {
    len_t len;  // Data's length
    void* ptr;  // Data
};
typedef struct kv_datum kv_datum;

// Allocation types
typedef enum { FIRST_FIT, WORST_FIT, BEST_FIT } alloc_t;

// File header sizes
#define HEADER_SIZE_H 5    // MagicN + hashFunctionIndex
#define HEADER_SIZE_BLK 5  // MagicN + nb_blocks
#define HEADER_SIZE_KV 1   // MagicN
#define HEADER_SIZE_DKV 5  // MagicN + nb_blocks

// Header size of blocks in BLK
#define HEADER_SIZE_BLOCK 9  // hasNextBlock(bool) + indexNextBlockWithSameHash(int) + numberOfSlots(int)

// Size of blocks in BLK
#define BLOCK_SIZE 4096

// Contains the file descriptors
typedef struct {
    int fd_h;          // File descriptor of h file
    int fd_blk;        // File descriptor of blk file
    int fd_kv;         // File descriptor of kv file
    int fd_dkv;        // File descriptor of dkv file
    const char* mode;  // File opening mode
} fileDescriptors;

// Structure to handle the currently loaded BLK block
typedef struct {
    unsigned char blockBLK[BLOCK_SIZE];  // Content of loaded block
    unsigned int indexCurrLoadedBlock;   // Index of currently loaded block
    unsigned int blockIsOccupiedSize;    // Size of the array tracking the currently occupied blocks
    bool* blockIsOccupied;   // Registers whether blocks are occupied or not. blockIsOccupied[i]=true means that block i
                             // is occupied
    unsigned int nb_blocks;  // Number of BLK blocks in the database
} blockHandler;

// Structure to manage the content of DKV
typedef struct {
    unsigned char* dkv;             // Holds the content of DKV
    unsigned int maxElementsInDKV;  // Maximal number of elements in DKV
    unsigned int nextTuple;         // Index of the next tuple in DKV (used to iterate over the database)
} dkvHandler;

// Structure to manage open databases
typedef struct {
    fileDescriptors fds;
    blockHandler bh;
    dkvHandler dkvh;
    alloc_t allocationMethod;  // Allocation method FIRST_FIT, WORST_FIT or BEST_FIT
    unsigned int (*hashFunction)(
        const kv_datum* kdatum);  // Hash function used to compute the hash of keys in the database
} KV;

#endif