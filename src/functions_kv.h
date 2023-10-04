#include "commonFunctions.h"
#include "structures.h"

int writeElementToKV(KV* kv, const kv_datum* key, const kv_datum* val, len_t offsetKV);

// Reads the key stored at position offsetKV into key. Returns 1 on success and -1 on failure.
int fillsKey(KV* kv, len_t offsetKV, kv_datum* key);
int fillValue(KV* kv, len_t offsetKV, kv_datum* val, const kv_datum* key);

// Returns the size of the key stored at offsetKV or 0 when error.
len_t getKeyLengthFromKV(KV* kv, len_t offsetKV);

int compareKeys(KV* kv, const kv_datum* key, len_t offsetKV);
