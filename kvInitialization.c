#include "kvInitialization.h"

/*r : O_RDONLY
r+ : O_CREAT|O_RDWR
w : O_CREAT|O_TRUNC|O_WRONLY
w+ : O_CREAT|O_RDWR
Returns NULL if doesn't succeed to open the database
*/
KV* kv_open(const char* dbname, const char* mode, int hashFunctionIndex, alloc_t alloc) {
    KV* kv = malloc(sizeof(KV));
    len_t nbElementsInDKV;

    if (initializeFiles(kv, mode, dbname, hashFunctionIndex) == -1)
        return NULL;

    unsigned int hashIndex;
    if (readAtPosition(kv->fds.fd_h, 1, &hashIndex, sizeof(unsigned int), kv) == -1)
        return NULL;
    kv->hashFunction = selectHashFunction(hashIndex);
    if (kv->hashFunction == NULL) {  // checks whether an hash function is associated to that index
        errno = EINVAL;
        return NULL;
    }

    if (readAtPosition(kv->fds.fd_blk, 1, &kv->bh.nb_blocks, sizeof(unsigned int), kv) == -1)
        return NULL;
    memset(kv->bh.blockBLK, 0, BLOCK_SIZE);
    // reads block 0 so that does not erase it at next call of readNewBlock
    if (readAtPosition(kv->fds.fd_blk, getOffsetBlk(0), kv->bh.blockBLK, BLOCK_SIZE, kv) == -1)
        return NULL;
    kv->bh.indexCurrLoadedBlock = 0;

    // adds space for 40 new blocks
    kv->bh.blockIsOccupied = calloc(kv->bh.nb_blocks + 40, sizeof(bool));
    kv->bh.blockIsOccupiedSize = 40 + kv->bh.nb_blocks;
    if (readsBlockOccupiedness(kv) == -1) {
        kv_close(kv);
        return NULL;
    }

    if (readAtPosition(kv->fds.fd_dkv, 1, &nbElementsInDKV, sizeof(int), kv) == -1)
        return NULL;
    // Adds space for 1000 elements
    kv->dkvh.dkv =
        calloc((nbElementsInDKV + 1000) * (sizeof(unsigned int) + sizeof(len_t)) + LG_EN_TETE_DKV, sizeof(char));
    if (readAtPosition(kv->fds.fd_dkv, 1, kv->dkvh.dkv,
                       nbElementsInDKV * (sizeof(unsigned int) + sizeof(len_t)) + LG_EN_TETE_DKV, kv) == -1)
        return NULL;
    kv->dkvh.maxElementsInDKV = nbElementsInDKV + 1000;

    kv->allocationMethod = alloc;
    kv->fds.mode = mode;
    return kv;
}

int initializeFiles(KV* database, const char* mode, const char* dbname, unsigned int hashFunctionIndex) {
    int databaseExists = openFiles(database, mode, dbname);

    if (strcmp(mode, "w") == 0 || strcmp(mode, "w+") == 0 || !databaseExists) {
        if (writeMagicNumbers(database) == -1)
            return -1;
        // Writes index of used hashFunction
        if (write_controle(database->fds.fd_h, &hashFunctionIndex, sizeof(unsigned int)) == -1)
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

// Fills the array whose entry i indicates whether block i is empty or not
int readsBlockOccupiedness(KV* kv) {
    int test;
    unsigned char buffblock[BLOCK_SIZE];
    for (unsigned int i = 0; i < kv->bh.blockIsOccupiedSize; i++) {
        if ((test = readAtPosition(kv->fds.fd_blk, getOffsetBlk(i + 1), buffblock, BLOCK_SIZE, kv)) == -1)
            return -1;

        if (test == 0)  // there is no block
            kv->bh.blockIsOccupied[i] = false;
        else {
            if (getNbElementsInBlockBLK(i, kv) == 0)  // No elements in block
                kv->bh.blockIsOccupied[i] = false;
            else
                kv->bh.blockIsOccupied[i] = true;
        }
    }
    return 1;
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

// Writes the magic number corresponding to each file
int writeMagicNumbers(KV* database) {
    unsigned char magicNumber = 1;
    if (write_controle(database->fds.fd_h, &magicNumber, 1) == -1)
        return -1;
    magicNumber = 2;
    if (write_controle(database->fds.fd_blk, &magicNumber, 1) == -1)
        return -1;
    magicNumber = 3;
    if (write_controle(database->fds.fd_kv, &magicNumber, 1) == -1)
        return -1;
    magicNumber = 4;
    if (write_controle(database->fds.fd_dkv, &magicNumber, 1) == -1)
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
