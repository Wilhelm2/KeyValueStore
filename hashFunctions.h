#ifndef HASHFUNCTIONS_H
#define HASHFUNCTIONS_H

#include "structures.h"

int (*choix_hachage(int index))(const kv_datum* clef);
int hachage_defaut(const kv_datum* clef);
int hachage_test(const kv_datum* clef);
int djb_hash(const kv_datum* clef);
int fnv_hash(const kv_datum* clef);

#endif