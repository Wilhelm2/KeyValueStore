#include "functions_kv.h"

int writeElementToKV(KV* kv, const kv_datum* key, const kv_datum* val, len_t offsetKV) {
    // First writes into an array to do only one write
    unsigned int nbBytesToWrite = sizeof(len_t) + key->len + sizeof(len_t) + val->len;
    unsigned char* tabwrite = malloc(nbBytesToWrite);
    // Writes key length + key
    memcpy(tabwrite, &key->len, sizeof(len_t));
    memcpy(tabwrite + sizeof(len_t), key->ptr, key->len);
    // Writes value length + value
    memcpy(tabwrite + sizeof(len_t) + key->len, &val->len, sizeof(len_t));
    memcpy(tabwrite + sizeof(len_t) + key->len + sizeof(len_t), val->ptr, val->len);
    printf("write key length %d key %.*s offset %d\n", key->len, key->len, (char*)key->ptr, offsetKV);

    if (writeAtPosition(kv->fds.fd_kv, offsetKV, tabwrite, nbBytesToWrite, kv) == -1)
        return -1;

    free(tabwrite);
    return 1;
}

int fillsKey(KV* kv, len_t offsetKV, kv_datum* key) {
    if (key == NULL) {  // key must have been allocated
        errno = EINVAL;
        return -1;
    }

    key->len = getKeyLengthFromKV(kv, offsetKV);
    if (key->len == 0)
        return -1;
    key->ptr = malloc(key->len);
    if (readAtPosition(kv->fds.fd_kv, offsetKV + sizeof(len_t), key->ptr, key->len, kv) == -1)
        return -1;
    return 1;
}

// Fills data into val
// Returns 1 on success and -1 on failure
int fillValue(KV* database, len_t offsetKV, kv_datum* val, const kv_datum* key) {
    if (val == NULL || key == NULL) {
        errno = EINVAL;
        return -1;
    }
    unsigned int posVal = offsetKV + sizeof(len_t) + key->len;
    if (readAtPosition(database->fds.fd_kv, posVal, &val->len, sizeof(len_t), database) == -1)
        return -1;
    val->ptr = malloc(val->len);
    if (readAtPosition(database->fds.fd_kv, posVal + sizeof(len_t), val->ptr, val->len, database) == -1)
        return -1;
    return 1;
}

len_t getKeyLengthFromKV(KV* kv, len_t offsetKV) {
    len_t length;
    if (readAtPosition(kv->fds.fd_kv, offsetKV, &length, sizeof(len_t), kv) == -1)
        return 0;
    return length;
}

// Returns 1 when keys are equal, -1 when error, and 0 otherwise
int compareKeys(KV* kv, const kv_datum* key, len_t offsetKV) {
    len_t keyLength = 0;
    len_t totalReadBytes = 0;
    int readBytes = 0;
    unsigned char buf[2048];
    unsigned int maxReadBytesInStep = 0;

    if (readAtPosition(kv->fds.fd_kv, offsetKV, &keyLength, sizeof(int), kv) == -1)
        return -1;
    //    printf("key->len %d keyLength %d offset %d\n", key->len, keyLength, offsetKV);

    if (keyLength != key->len)
        return 0;
    while (totalReadBytes < keyLength) {
        maxReadBytesInStep = (keyLength - totalReadBytes) % 2049;  // reads at most 2048 bytes at each step
        readBytes = read_controle(kv->fds.fd_kv, buf, maxReadBytesInStep);
        if (readBytes == -1)
            return -1;
        //      printf("length %d key %.*s offset %d\n", readBytes, readBytes, buf, offsetKV);

        if (memcmp(((unsigned char*)key->ptr) + totalReadBytes, buf, readBytes) != 0)
            return 0;  // part of keys are not equal
        totalReadBytes += (len_t)readBytes;
    }
    return 1;
}