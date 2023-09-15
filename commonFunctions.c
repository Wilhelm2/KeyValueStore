#include "commonFunctions.h"

// Concat two strings
char* concat(const char* S1, const char* S2) {
    char* result = malloc(strlen(S1) + strlen(S2) + 1);
    strncpy(result, S1, strlen(S1));
    strncpy(result + strlen(S1), S2, strlen(S2));
    result[strlen(S1) + strlen(S2)] = '\0';
    return result;
}

// Reads bytes while controling for errors and that the right number of bytes have been read
int read_controle(int descripteur, void* ptr, int nboctets) {
    int test;
    if ((test = read(descripteur, ptr, nboctets)) == -1)
        return -1;
    if (test != nboctets) {
        errno = EINVAL;
        return -1;
    }
    return test;
}

// Writes data while controlling for errors and that the right number of bytes have been written
int write_controle(int descripteur, const void* ptr, int nboctets) {
    int test;
    if ((test = write(descripteur, ptr, nboctets)) == -1)
        return -1;
    if (test != nboctets) {
        errno = EINVAL;
        return -1;
    }
    return nboctets;
}

int readAtPosition(int fd, unsigned int position, void* dest, unsigned int nbBytes, KV* database) {
    int readBytes;
    if (lseek(fd, position, SEEK_SET) == -1) {
        closeFileDescriptors(database);
        free(database);
        return -1;
    }

    // lecture de dkv en mémoire
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

    // lecture de dkv en mémoire
    if (write_controle(fd, src, nbBytes) == -1) {
        closeFileDescriptors(database);
        free(database);
        return -1;
    }
    return 1;
}

// Reads a new block by first writing the previous one to the BLK file
int readNewBlock(unsigned int index, KV* database) {
    if (writeAtPosition(database->fds.fd_blk, getOffsetBlk(database->bh.indexCurrLoadedBlock), database->bh.block,
                        BLOCK_SIZE, database) == -1)
        return -1;
    memset(database->bh.block, 0, BLOCK_SIZE);
    database->bh.indexCurrLoadedBlock = index;
    if (readAtPosition(database->fds.fd_blk, getOffsetBlk(index), database->bh.block, BLOCK_SIZE, database) == -1)
        return -1;
    return 1;
}

// Close either returns 0 when succeeds or -1 when fails. Thus the function will return -1 if one of the closes fail
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
    return res;
}
