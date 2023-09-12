#ifndef SLOT_ALLOCATIONS_H
#define SLOT_ALLOCATIONS_H
#include "commonFunctions.h"
#include "structures.h"
// Functions used to allocate database slots
int (*choix_allocation(KV* database))(KV* database, const kv_datum* key, const kv_datum* val);
int first_fit(KV* kv, const kv_datum* key, const kv_datum* val);
int best_fit(KV* kv, const kv_datum* key, const kv_datum* val);
int worst_fit(KV* kv, const kv_datum* key, const kv_datum* val);

#endif