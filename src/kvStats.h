#include "commonFunctions.h"
#include "kv.h"
#include "structures.h"

void verifyEntriesDKV(KV* database);
void printStatsOnDKV(KV* database);
void printSlotsDKV(KV* database);
void printFreeSlotsDKV(KV* database);
void printTakenSlotsDKV(KV* database);
void printElementsPerBlock(KV* database);
void averageKeyLength(KV* database);
void averageValueLength(KV* database);