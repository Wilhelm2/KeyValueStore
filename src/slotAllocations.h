#ifndef SLOT_ALLOCATIONS_H
#define SLOT_ALLOCATIONS_H
#include "commonFunctions.h"
#include "structures.h"

// Returns the allocation method used by the database
int (*getAllocationMethod(KV* database))(KV* database, const kv_datum* key, const kv_datum* val);
// Returns the first available slot big enough to hold key+value. Returns -1 if no such available slot exists.
int first_fit(KV* kv, const kv_datum* key, const kv_datum* val);
// Returns the biggest available slot that can hold key+value. Returns -1 if no such available slot exists.
int best_fit(KV* kv, const kv_datum* key, const kv_datum* val);
// Returns the smallest slot among the available slots that are big enough to hold key+value.
// Returns -1 if no such available slot exists.
int worst_fit(KV* kv, const kv_datum* key, const kv_datum* val);

#endif