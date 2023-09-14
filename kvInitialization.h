#ifndef KV_INITIALIZATION_H
#define KV_INITIALIZATION_H
// #include <errno.h>
// #include <fcntl.h>
// #include <stdio.h>
// #include <stdlib.h>
// #include <string.h>

#include "commonFunctions.h"
#include "hashFunctions.h"
#include "slotAllocations.h"
#include "structures.h"

KV* kv_open(const char* dbname, const char* mode, int hashFunctionIndex, alloc_t alloc);
int initializeFiles(KV* database, const char* mode, const char* dbname, unsigned int hashFunctionIndex);
int openFiles(KV* database, const char* mode, const char* dbname);
int readsBlockOccupiedness(KV* kv);
int getOpenMode(const char* mode);
int writeMagicNumbers(KV* database);
int verifyFileMagicNumber(int descr, unsigned char magicNumber, KV* database);
int tryOpen(KV* database, const char* filename, int flags, unsigned int mode);
int kv_close(KV* kv);

#endif