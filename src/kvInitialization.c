#include "kvInitialization.h"
int initializeFiles(KV* database, const char* mode, const char* dbname, unsigned int hashFunctionIndex) {
    int databaseExists = openFiles(database, mode, dbname);

    if (strcmp(mode, "w") == 0 || strcmp(mode, "w+") == 0 || !databaseExists) {
        if (writeMagicNumbers(database) == -1)
            return -1;
        // Writes index of used hashFunction
        if (writeAtPosition(database->fds.fd_h, 1, &hashFunctionIndex, sizeof(unsigned int), database) == -1)
            return -1;
        char tmp[HASH_TABLE_SIZE];
        memset(tmp, -1, HASH_TABLE_SIZE);
        // Required because otherwise will read blockIndex = 0 when there is no blockIndex written
        if (writeAtPosition(database->fds.fd_h, HEADER_SIZE_H, tmp, HASH_TABLE_SIZE, database) == -1)
            return -1;

        // Sets the number of blocks and slots to 0
        unsigned int j = 0;
        if (writeAtPosition(database->fds.fd_dkv, 1, &j, 4, database) == -1)
            return -1;
        if (writeAtPosition(database->fds.fd_blk, 1, &j, 4, database) == -1)
            return -1;
    }

    // Controls that all files have the right magic number
    if (verifyFileMagicNumber(database->fds.fd_h, 1, database) != 1 ||
        verifyFileMagicNumber(database->fds.fd_blk, 2, database) != 1 ||
        verifyFileMagicNumber(database->fds.fd_kv, 3, database) != 1 ||
        verifyFileMagicNumber(database->fds.fd_dkv, 4, database) != 1) {
        closeFileDescriptors(database);
        free(database);
        return -1;
    }
    return databaseExists;
}

int openFiles(KV* database, const char* mode, const char* dbname) {
    int flags = getOpenMode(mode);
    int databaseExists;
    int tmpfd;
    char* dbnh = concat(dbname, ".h");

    if ((tmpfd = open(dbnh, O_RDONLY, 0666)) == -1) {
        // Database doesn't exist and tried to open it in read mode or other reason than not existing database
        if (errno != ENOENT || strcmp(mode, "r") == 0) {
            free(dbnh);
            close(tmpfd);
            free(database);
            return -1;
        }
        databaseExists = 0;  // database does not exist
    } else
        databaseExists = 1;  // database exists
    close(tmpfd);

    database->fds.fd_h = tryOpen(database, dbnh, flags, 0666);
    free(dbnh);
    if (database->fds.fd_h == -1)
        return -1;

    char* dbnblk = concat(dbname, ".blk");
    database->fds.fd_blk = tryOpen(database, dbnblk, flags, 0666);
    free(dbnblk);
    if (database->fds.fd_blk == -1)
        return -1;

    char* dbnkv = concat(dbname, ".kv");
    database->fds.fd_kv = tryOpen(database, dbnkv, flags, 0666);
    free(dbnkv);
    if (database->fds.fd_kv == -1)
        return -1;

    char* dbndkv = concat(dbname, ".dkv");
    database->fds.fd_dkv = tryOpen(database, dbndkv, flags, 0666);
    free(dbndkv);
    if (database->fds.fd_dkv == -1)
        return -1;
    return databaseExists;
}

int readsBlockOccupiedness(KV* kv) {
    int test;
    unsigned char buffblock[BLOCK_SIZE];
    for (unsigned int i = 0; i < kv->bh.blockIsOccupiedSize; i++) {
        if ((test = readAtPosition(kv->fds.fd_blk, getOffsetBlk(i + 1), buffblock, BLOCK_SIZE, kv)) == -1)
            return -1;

        if (test == 0)  // there is no block
            kv->bh.blockIsOccupied[i] = false;
        else {
            if (getNbSlotsInBLKBlock(i, kv) == 0)  // No elements in block
                kv->bh.blockIsOccupied[i] = false;
            else
                kv->bh.blockIsOccupied[i] = true;
        }
    }
    return 1;
}

int getOpenMode(const char* mode) {
    int flags;
    if (strcmp(mode, "r") == 0)
        flags = O_RDONLY;
    if (strcmp(mode, "r+") == 0)
        flags = O_CREAT | O_RDWR;
    if (strcmp(mode, "w") == 0)
        flags = O_CREAT | O_TRUNC | O_RDWR;
    if (strcmp(mode, "w+") == 0)
        flags = O_CREAT | O_TRUNC | O_RDWR;
    return flags;
}

int writeMagicNumbers(KV* database) {
    unsigned char magicNumber = 1;
    if (writeAtPosition(database->fds.fd_h, 0, &magicNumber, 1, database) == -1)
        return -1;
    magicNumber = 2;
    if (writeAtPosition(database->fds.fd_blk, 0, &magicNumber, 1, database) == -1)
        return -1;
    magicNumber = 3;
    if (writeAtPosition(database->fds.fd_kv, 0, &magicNumber, 1, database) == -1)
        return -1;
    magicNumber = 4;
    if (writeAtPosition(database->fds.fd_dkv, 0, &magicNumber, 1, database) == -1)
        return -1;
    return 1;
}

int verifyFileMagicNumber(int descr, unsigned char magicNumber, KV* database) {
    unsigned char storedMagicNumber;
    if (readAtPosition(descr, 0, &storedMagicNumber, 1, database) == -1)
        return -1;
    if (storedMagicNumber != magicNumber)
        return 0;
    return 1;
}

int tryOpen(KV* database, const char* filename, int flags, unsigned int mode) {
    int fd = open(filename, flags, mode);
    if (fd == -1) {
        closeFileDescriptors(database);
        free(database);
    }
    return fd;
}
