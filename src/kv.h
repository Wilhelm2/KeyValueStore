#ifndef KV_H
#define KV_H

#include "functions_blk.h"
#include "functions_dkv.h"
#include "functions_h.h"
#include "functions_kv.h"
#include "hashFunctions.h"
#include "kvInitialization.h"
#include "slotAllocations.h"
#include "structures.h"

/* Opens the database. The database can be opened in the modes:
    - r : O_RDONLY
    - r+ : O_CREAT|O_RDWR
    - w : O_CREAT|O_TRUNC|O_WRONLY
    - w+ : O_CREAT|O_RDWR
    Returns NULL if fails to open the database. Otherwise returns KV, the structure containing the database's metadata.
*/
KV* kv_open(const char* dbname, const char* mode, int hashFunctionIndex, alloc_t alloc);
// Closes the database. Returns -1 upon failure and 1 on success.
int kv_close(KV* kv);
// Adds an element into the database. The element is identified by its key.
// If a couple (key,val1) is already registered, then deletes it and replaces it with (key,val).
int kv_put(KV* kv, const kv_datum* key, const kv_datum* val);
// Gets the value associated to key into val.
// Returns 1 on success, 0 if the key is not present in the database and -1 on error.
int kv_get(KV* kv, const kv_datum* key, kv_datum* val);
// Deletes a key from the database.
// Returns 0 on success, -1 with on error, with errno set to EINVAL if the key is not contained in the database.
int kv_del(KV* kv, const kv_datum* key);
// Initializes the database's iterator.
void kv_start(KV* kv);
// Gets the next couple (key,value) in the database.
// Returns 1 when succeeded to read the next couple, 0 when arrived at the end of the database, and -1 on error.
int kv_next(KV* kv, kv_datum* key, kv_datum* val);
// Truncates KV by removing obsolete data taken up by deleted or moved data.
int truncate_kv(KV* kv);

#endif