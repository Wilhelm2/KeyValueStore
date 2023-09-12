#include "kvInitialization.h"

int initializeFiles(KV* database, const char* mode, const char* dbname, unsigned int hashFunctionIndex) {
    int databaseExists = openFiles(database, mode, dbname);

    if (strcmp(mode, "w") == 0 || strcmp(mode, "w+") == 0 || !databaseExists) {
        if (writeMagicNumbers(database) == -1)
            return -1;
        // Writes index of used hashFunction
        if (write_controle(database->fd_h, &hashFunctionIndex, sizeof(unsigned int)) == -1)
            return -1;
        // Sets the number of blocs and slots to 0
        unsigned int j = 0;
        if (writeAtPosition(database->fd_dkv, 1, &j, 4, database) == -1)
            return -1;
        if (writeAtPosition(database->fd_blk, 1, &j, 4, database) == -1)
            return -1;
    }

    // Controls that all files have the right magic number
    if (verifyFileMagicNumber(database->fd_h, 1, database) != 1 ||
        verifyFileMagicNumber(database->fd_blk, 2, database) != 1 ||
        verifyFileMagicNumber(database->fd_kv, 3, database) != 1 ||
        verifyFileMagicNumber(database->fd_dkv, 4, database) != 1) {
        closeFileDescriptors(database);
        free(database);
        return -1;
    }
    return databaseExists;
}

// Get opening mode of file following mode. Opening modes are: r,r+,w,w+
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

// Open (or creates) all database files
// Returns -1 on errors, 0 when the database didn't exist, and 1 when the database already existed
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

    database->fd_h = tryOpen(database, dbnh, flags, 0666);
    free(dbnh);
    if (database->fd_h == -1)
        return -1;

    char* dbnblk = concat(dbname, ".blk");
    database->fd_blk = tryOpen(database, dbnblk, flags, 0666);
    free(dbnblk);
    if (database->fd_blk == -1)
        return -1;

    char* dbnkv = concat(dbname, ".kv");
    database->fd_kv = tryOpen(database, dbnkv, flags, 0666);
    free(dbnkv);
    if (database->fd_kv == -1)
        return -1;

    char* dbndkv = concat(dbname, ".dkv");
    database->fd_dkv = tryOpen(database, dbndkv, flags, 0666);
    free(dbndkv);
    if (database->fd_dkv == -1)
        return -1;
    return databaseExists;
}

// Writes the magic number corresponding to each file
int writeMagicNumbers(KV* database) {
    unsigned char magicNumber = 1;
    if (write_controle(database->fd_h, &magicNumber, 1) == -1)
        return -1;
    magicNumber = 2;
    if (write_controle(database->fd_blk, &magicNumber, 1) == -1)
        return -1;
    magicNumber = 3;
    if (write_controle(database->fd_kv, &magicNumber, 1) == -1)
        return -1;
    magicNumber = 4;
    if (write_controle(database->fd_dkv, &magicNumber, 1) == -1)
        return -1;
    return 1;
}

// Verifies that the file contain the right magic number
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
