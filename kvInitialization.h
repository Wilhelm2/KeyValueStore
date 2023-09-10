#ifndef DATABASEINITIALIZATION_H
#define DATABASEINITIALIZATION_H

#include "commonFunctions.h"
#include "structures.h"

int initializeFileDescriptors(KV* database, const char* mode, const char* dbname);
int getOpenMode(const char* mode);
int verificationMagicN(int descr, const char* mode, unsigned char magicNumber, int databaseExists);
int tryOpen(KV* database, const char* filename, int flags, unsigned int mode);

#endif