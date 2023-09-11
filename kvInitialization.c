#include "kvInitialization.h"

int initializeFileDescriptors(KV* database, const char* mode, const char* dbname) {
    int flag = getOpenMode(mode);
    int databaseExits;

    char* dbnh = concat(dbname, ".h");
    if (open(dbnh, O_RDONLY, 0666) == -1) {
        // Database doesn't exist and tried to open it in read mode or other reason than not existing database
        if (errno != ENOENT || strcmp(mode, "r") == 0) {
            free(dbnh);
            free(database);
            return -1;
        }
        databaseExits = 0;  // database does not exist
    } else
        databaseExits = 1;  // database exists

    database->descr_h = tryOpen(database, dbnh, flag, 0666);
    free(dbnh);
    if (database->descr_h == -1)
        return -1;

    char* dbnblk = concat(dbname, ".blk");
    database->descr_blk = tryOpen(database, dbnblk, flag, 0666);
    free(dbnblk);
    if (database->descr_blk == -1)
        return -1;

    char* dbnkv = concat(dbname, ".kv");
    database->descr_kv = tryOpen(database, dbnkv, flag, 0666);
    free(dbnkv);
    if (database->descr_kv == -1)
        return -1;

    char* dbndkv = concat(dbname, ".dkv");
    database->descr_dkv = tryOpen(database, dbndkv, flag, 0666);
    free(dbndkv);
    if (database->descr_dkv == -1)
        return -1;

    // Controls that all files have the right magic number
    if (verificationMagicN(database->descr_h, mode, '1', databaseExits) != 1 ||
        verificationMagicN(database->descr_blk, mode, '2', databaseExits) != 1 ||
        verificationMagicN(database->descr_kv, mode, '3', databaseExits) != 1 ||
        verificationMagicN(database->descr_dkv, mode, '4', databaseExits) != 1) {
        closeFileDescriptors(database);
        free(database);
        return -1;
    }

    return databaseExits;
}

// Get opening mode of file following mode. Opening modes are: r,r+,w,w+
int getOpenMode(const char* mode) {
    int flag;
    if (strcmp(mode, "r") == 0)
        flag = O_RDONLY;
    if (strcmp(mode, "r+") == 0)
        flag = O_CREAT | O_RDWR;
    if (strcmp(mode, "w") == 0)
        flag = O_CREAT | O_TRUNC | O_RDWR;
    if (strcmp(mode, "w+") == 0)
        flag = O_CREAT | O_TRUNC | O_RDWR;
    return flag;
}

// Verifies that files contain the right magic numbers
int verificationMagicN(int descr, const char* mode, unsigned char magicNumber, int databaseExists) {
    unsigned char storedMagicNumber;
    if (strcmp(mode, "w") == 0 || strcmp(mode, "w+") == 0 || !databaseExists) {
        // No verification required because database truncated or just created
        write_controle(descr, &magicNumber, 1);
    } else if ((strcmp(mode, "r") == 0 || strcmp(mode, "r+") == 0) && databaseExists) {
        // Verifies that file contains right magic number
        if (read_controle(descr, &storedMagicNumber, 1) == -1)
            return -1;
        if (storedMagicNumber != magicNumber)
            return 0;
    }
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
