#include "commonFunctions.h"
#include "structures.h"

// Writes the couple (key,val) to KV at offset offsetKV.
int writeElementToKV(KV* kv, const kv_datum* key, const kv_datum* val, len_t offsetKV);

// Reads the key contained at offsetKV into key. Key must have been allocated before.
// Returns 1 on success and -1 on failure, with errno = EINVAL if key was not allocated.
int readKey(KV* kv, len_t offsetKV, kv_datum* key);
// Reads the value contained at offsetKV into key. Val and key must have been allocated before.
// Returns 1 on success and -1 on failure, with errno = EINVAL if one of both was not allocated.
int readValue(KV* kv, len_t offsetKV, kv_datum* val, const kv_datum* key);
// Reads the key length of the key stored at offsetKV. Returns the key's length on success and 0 on failure.
len_t getKeyLengthFromKV(KV* kv, len_t offsetKV);
// Reads the value length of the value stored at offsetKV. Returns the value's length on success and 0 on failure.
len_t getValueLengthFromKV(KV* kv, len_t offsetKV);
// Compares two keys, which are the key given as argument and the key stored at offsetKV in KV.
// Returns 1 when the keys are equal, 0 if they are different, and -1 on error.
int compareKeys(KV* kv, const kv_datum* key, len_t offsetKV);

// Returns the offset of index in KV.
static inline len_t getOffsetKV(unsigned int index) {
    return HEADER_SIZE_KV + index;
}
