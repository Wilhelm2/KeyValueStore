#include "commonFunctions.h"
#include "kv.h"
#include "structures.h"

// Verifies the integrity of DKV, i.e., that no space is lost.
void verifyEntriesDKV(KV* database);
// Prints stats from DKV relative to the database's used and free slots.
void printStatsOnDKV(KV* database);
// Print stats about DKV slots: FREE/TAKEN - offset in KV - length.
void printSlotsDKV(KV* database);
// Print stats about the free DKV slots: FREE/TAKEN - offset in KV - length.
void printFreeSlotsDKV(KV* database);
// Print stats about the taken DKV slots: FREE/TAKEN - offset in KV - length.
void printTakenSlotsDKV(KV* database);
// Prints the number of elements per block series in BLK. A series of block is constituted from a block and the list of
// its next blocks, i.e., from the blocks that point to keys that have the same hash value.
void printElementsPerBlock(KV* database);
// Prints the average length of keys.
void averageKeyLength(KV* database);
// Prints the average length of values.
void averageValueLength(KV* database);