#ifndef KV_INITIALIZATION_H
#define KV_INITIALIZATION_H
// #include <errno.h>
// #include <fcntl.h>
// #include <stdio.h>
// #include <stdlib.h>
// #include <string.h>

#include "commonFunctions.h"
#include "structures.h"

typedef struct s_KV KV;

int initializeFileDescriptors(KV* database, const char* mode, const char* dbname);
int getOpenMode(const char* mode);
int verificationMagicN(int descr, const char* mode, unsigned char magicNumber, int databaseExists);
int tryOpen(KV* database, const char* filename, int flags, unsigned int mode);
#endif