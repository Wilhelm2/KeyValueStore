#ifndef COMMON_FUNCTIONS_H
#define COMMON_FUNCTIONS_H

#include "structures.h"

// Concatenates two strings. Allocates the space necessary to contain the two strings.
char* concat(const char* S1, const char* S2);
// Reads nbBytes from fd into ptr. Returns the number of read bytes on success of reading nbBytes.
// Returns -1 on failure, with errno set to EINVAL if couldn't read nbBytes.
int readControlled(int fd, void* ptr, int nbBytes);
// Writes nbBytes from ptr into fd. Returns the number of written bytes on success of writing nbBytes.
// Returns -1 on failure, with errno set to EINVAL if couldn't write nbBytes.
int writeControlled(int fd, const void* ptr, int nbBytes);
// Reads nbBytes at position position of fd into dest.
// Returns the number of read bytes on success and -1 on failure.
int readAtPosition(int fd, unsigned int position, void* dest, unsigned int nbBytes, KV* database);
// Writes nbBytes at position position of fd into dest.
// Returns 1 on success and -1 on failure.
int writeAtPosition(int fd, unsigned int position, void* src, unsigned int nbBytes, KV* database);
// Closes the file descriptors associated to the database. Returns 0 on success of closing all files, and -1 otherwise.
int closeFileDescriptors(KV* database);
// Frees the memory allocated to the structure that manages the database.
void freeDatabase(KV* database);
#endif