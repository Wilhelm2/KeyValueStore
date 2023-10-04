#ifndef KV_INITIALIZATION_H
#define KV_INITIALIZATION_H
// #include <errno.h>
// #include <fcntl.h>
// #include <stdio.h>
// #include <stdlib.h>
// #include <string.h>

#include "commonFunctions.h"
#include "functions_blk.h"
#include "hashFunctions.h"
#include "slotAllocations.h"
#include "structures.h"

// Initializes the file descriptors and verifies that the right magic numbers are saved in the files when the database
// already exists. Returns -1 upon error or when a file has not the right magic number.
int initializeFiles(KV* database, const char* mode, const char* dbname, unsigned int hashFunctionIndex);
// Open or create database files. Returns -1 on errors, 0 if database doesn't exist, and 1 if database already existed.
int openFiles(KV* database, const char* mode, const char* dbname);
// Fills the array whose entry i indicates whether block i is empty or not.
int readsBlockOccupiedness(KV* kv);
// Get the file opening mode for "open()" following mode. The opening modes are: r,r+,w,w+.
int getOpenMode(const char* mode);
// Writes the magic number into each file.
int writeMagicNumbers(KV* database);
// Verifies that the file contains the right magic number. Returns -1 on error, 0 if the number is not corresponding
// to the number saved into the file and 1 if the numbers are corresponding.
int verifyFileMagicNumber(int descr, unsigned char magicNumber, KV* database);
// Tries to open the file filename with the flags flags in mode mode. Returns file descriptor on success and -1 on
// error.
int tryOpen(KV* database, const char* filename, int flags, unsigned int mode);

#endif