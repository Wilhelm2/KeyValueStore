#include "commonFunctions.h"

char* concat(const char* S1, const char* S2) {
    char* result = malloc(strlen(S1) + strlen(S2) + 1);
    strncpy(result, S1, strlen(S1));
    strncpy(result + strlen(S1), S2, strlen(S2));
    result[strlen(S1) + strlen(S2)] = '\0';
    return result;
}

int readControlled(int fd, void* ptr, int nbBytes) {
    int test;
    if ((test = read(fd, ptr, nbBytes)) == -1)
        return -1;
    if (test != nbBytes) {
        errno = EINVAL;
        return -1;
    }
    return test;
}

int writeControlled(int fd, const void* ptr, int nbBytes) {
    int test;
    if ((test = write(fd, ptr, nbBytes)) == -1)
        return -1;
    if (test != nbBytes) {
        errno = EINVAL;
        return -1;
    }
    return nbBytes;
}

int readAtPosition(int fd, unsigned int position, void* dest, unsigned int nbBytes, KV* database) {
    int readBytes;
    if (lseek(fd, position, SEEK_SET) == -1) {
        closeFileDescriptors(database);
        free(database);
        return -1;
    }
    if ((readBytes = read(fd, dest, nbBytes)) == -1) {
        closeFileDescriptors(database);
        free(database);
        return -1;
    }
    return readBytes;
}

int writeAtPosition(int fd, unsigned int position, void* src, unsigned int nbBytes, KV* database) {
    if (lseek(fd, position, SEEK_SET) == -1) {
        closeFileDescriptors(database);
        free(database);
        return -1;
    }
    if (writeControlled(fd, src, nbBytes) == -1) {
        closeFileDescriptors(database);
        free(database);
        return -1;
    }
    return 1;
}

int closeFileDescriptors(KV* database) {
    int res = 0;
    if (database->fds.fd_blk > 0)
        res |= close(database->fds.fd_blk);
    if (database->fds.fd_dkv > 0)
        res |= close(database->fds.fd_dkv);
    if (database->fds.fd_h > 0)
        res |= close(database->fds.fd_h);
    if (database->fds.fd_kv > 0)
        res |= close(database->fds.fd_kv);
    // Close either returns 0 when succeeds or -1 when fails.
    // Thus the function will return -1 if one of the closes fail.
    return res;
}

void freeDatabase(KV* database) {
    free(database->dkvh.dkv);
    free(database->bh.blockIsOccupied);
    free(database);
}
