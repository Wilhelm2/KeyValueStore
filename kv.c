// WILHELM Daniel YAMAN Kendal

#include "kv.h"
/*r : O_RDONLY
r+ : O_CREAT|O_RDWR
w : O_CREAT|O_TRUNC|O_WRONLY
w+ : O_CREAT|O_RDWR
*/
// retourne NULL si n'arrive pas à ouvrir la base de donnée
KV* kv_open(const char* dbname, const char* mode, int hidx, alloc_t alloc) {
    KV* kv = malloc(sizeof(struct s_KV));
    len_t nbElementsInDKV;
    int (*hashFunction)(const kv_datum* kdatum);

    if (initializeFiles(kv, mode, dbname, hidx) == -1)
        return NULL;

    if (readAtPosition(kv->fd_dkv, 1, &nbElementsInDKV, sizeof(int), kv) == -1)
        return NULL;

    if (readAtPosition(kv->fd_blk, 1, &kv->nb_blocs, sizeof(unsigned int), kv) == -1)
        return NULL;

    // adds space for 40 other blocs
    kv->blocIsOccupied = calloc(kv->nb_blocs + 40, sizeof(bool));

    // Adds space for 1000 elements
    kv->dkv = malloc((nbElementsInDKV + 1000) * (sizeof(unsigned int) + sizeof(len_t)) + LG_EN_TETE_DKV);
    if (readAtPosition(kv->fd_dkv, 1, kv->dkv, sizeOfDKVFilled(kv), kv) == -1)
        return NULL;
    kv->maxElementsInDKV = nbElementsInDKV + 1000;

    unsigned int indexbase;
    if (readAtPosition(kv->fd_h, 1, &indexbase, sizeof(unsigned int), kv) == -1)
        return NULL;

    hashFunction = selectHashFunction(indexbase);
    if (hashFunction == NULL)  // checks whether an hash function is associated to that index
    {
        errno = EINVAL;
        return NULL;
    }

    kv->blocIsOccupiedSize = 40 + kv->nb_blocs;
    if (remplit_bloc(kv) == -1) {
        kv_close(kv);
        return NULL;
    }

    kv->allocationMethod = alloc;
    kv->mode = mode;
    kv->hashFunction = hashFunction;
    return kv;
}

// ferme la base de donnée, retourne -1 si échoue, sinon retourne 0
int kv_close(KV* kv) {
    int res = 0;

    if (strcmp(kv->mode, "r") != 0) {
        // écrture de dkv
        if (writeAtPosition(kv->fd_dkv, 0, kv->dkv, sizeOfDKVFilled(kv), kv) == -1) {
            freeDatabase(kv);
            return -1;
        }

        if (writeAtPosition(kv->fd_blk, 1, &kv->nb_blocs, sizeof(unsigned int), kv) == -1) {
            freeDatabase(kv);
            return -1;
        }

        // met .dkv à la bonne taille : LG_EN_TETE_DKV + nb_emplacements*8
        // pas besoin de fstat vu qu'on a toutes les données
        if (ftruncate(kv->fd_dkv, sizeOfDKVFilled(kv)) == -1) {
            perror("ftruncate dkv");
            exit(1);
        }
    }

    if (truncate_kv(kv) == -1)
        res = -1;
    res |= closeFileDescriptors(kv);
    freeDatabase(kv);
    return res;
}

void freeDatabase(KV* database) {
    free(database->dkv);
    free(database->blocIsOccupied);
    free(database);
}

// fonction qui recherche la valeur associée a key
// Retour : 1 trouvé, 0 non trouvé, -1 erreur
int kv_get(KV* kv, const kv_datum* key, kv_datum* val) {
    int hash = kv->hashFunction(key);
    int numbloc;
    len_t offset;
    if (strcmp(kv->mode, "w") == 0) {  // not allowed to read
        errno = EACCES;
        return -1;
    }

    if ((numbloc = RechercheBlocH(kv, hash)) < 1) {  // either no bloc associated  to that hash or error
        if (numbloc == 0)                            // no bloc
            errno = EINVAL;
        return numbloc;
    }

    // recherche de la clef dans .kv, retourne l'offset si trouvé, -1 sinon
    offset = RechercheOffsetClef(kv, key, numbloc);
    if (offset == 0)
        return 0;
    else
        offset--;  // enlève de nouveau le +1 ajouté dans RechercheOffsetClef
    if (RemplissageVal(kv, offset, val, key) == -1)
        return -1;

    if (val == NULL)
        return -1;
    return 1;
}

int kv_put(KV* kv, const kv_datum* key, const kv_datum* val) {
    int hash = kv->hashFunction(key);
    int numbloc;
    int emplacement_dkv;
    int test;
    // Check right to write
    if (strcmp(kv->mode, "r") == 0) {
        errno = EACCES;
        return -1;
    }

    // First looks the block up. The function returns 0 when the hash is associated to no block
    if ((numbloc = RechercheBlocH(kv, hash)) == 0) {
        // Allocate a block to the hash value
        numbloc = Allouebloc(kv);
        if (numbloc == -1)
            return -1;
        if (liaisonHBlk(kv, hash, numbloc) == -1)  // liaison entre .h et .blk
            return -1;
    }
    if (numbloc == -1)
        return -1;
    // recherche d'abord si la clef est déjà présente dans la base et la supprime
    test = RechercheOffsetClef(kv, key, numbloc);
    if (test == 0 && errno != EINVAL)  // dans ce cas il y a eu une erreur
        return -1;
    if (test != 0)  // clef déjà présente
    {
        // dans ce cas la clef est déjà contenue dans la base donc on del
        kv_del(kv, key);
        // fait les même tests pour refaire le lien si del a supprimé le bloc
        if ((numbloc = RechercheBlocH(kv, hash)) == 0) {
            numbloc = Allouebloc(kv);  // realloue un bloc
            if (numbloc == -1)
                return -1;
            if (liaisonHBlk(kv, hash, numbloc) == -1)  // liaison entre .h et .blk
                return -1;
        }
        if (numbloc == -1)
            return -1;
    }

    // recherche d'un emplacement pour stocker la valeur
    if ((emplacement_dkv = choix_allocation(kv)(kv, key, val)) == -1) {  // créé un nouvel emplacement dans dkv
        NouvEmplacementDkv(kv, key, val);
        emplacement_dkv = *(int*)(kv->dkv) - 1;
    }

    len_t offset = access_offset_dkv(emplacement_dkv, kv);
    writeElementToKV(kv, key, val, offset);

    // établissement de la liaison entre .blk et .kv
    if (lienBlkKv(numbloc, kv, offset) == -1)
        return -1;

    // met l'emplacement en tant que place occupée dans dkv
    kv = SetOccupeDkv(kv, emplacement_dkv);
    // voit s'il y a encore de la place pour un autre couple et l'insère
    insertionNewEspace(kv, emplacement_dkv, key, val);
    return 0;
}

int writeElementToKV(KV* kv, const kv_datum* key, const kv_datum* val, len_t offset) {
    // First writes into an array to do only write
    unsigned int nbBytesToWrite = sizeof(len_t) + key->len + sizeof(len_t) + val->len;
    unsigned char* tabwrite = malloc(nbBytesToWrite);
    // Writes key length + key
    memcpy(tabwrite, &key->len, sizeof(len_t));
    memcpy(tabwrite + sizeof(len_t), key->ptr, key->len);
    // Writes value length + value
    memcpy(tabwrite + sizeof(len_t) + key->len, &val->len, sizeof(len_t));
    memcpy(tabwrite + sizeof(len_t) + key->len + sizeof(len_t), val->ptr, val->len);

    if (writeAtPosition(kv->fd_kv, getOffsetKV(offset), tabwrite, nbBytesToWrite, kv) == -1)
        return -1;

    free(tabwrite);
    return 1;
}

int kv_del(KV* kv, const kv_datum* key) {
    int hash = kv->hashFunction(key);
    int numbloc;
    len_t offset;
    int test;
    // vérifie que la base a été ouverte avec un mode autorisé
    if (strcmp(kv->mode, "r") == 0) {
        errno = EACCES;
        return -1;
    }

    if ((test = readAtPosition(kv->fd_h, getOffsetH(hash), &numbloc, sizeof(int), kv)) == -1)
        return -1;

    // si la fonction retourne 0 alors il n'y a pas de bloc avec cette valeur
    if (test == 0 || numbloc == 0) {
        errno = ENOENT;
        return -1;
    }

    // lit le bloc
    if (readAtPosition(kv->fd_blk, getOffsetBlk(numbloc), kv->bloc, BLOCK_SIZE, kv) == -1)
        return -1;

    offset = RechercheOffsetClef(kv, key, numbloc);
    if (offset == 0 && errno != EINVAL)
        return -1;
    else if (offset == 0)  // key not contained
    {
        errno = ENOENT;
        return -1;
    }
    offset--;  // corrige l'offset renvoyé par RechercheOffsetClef

    // la clef est contenue dans kv et son offset est offset, on la supprime
    libereEmplacementdkv(offset, kv);
    if (libereEmplacementblk(numbloc, offset, kv, hash, 0) == -1)
        return -1;
    return 0;
}

void kv_start(KV* kv) {
    // si mode = w alors pas le droit de lire dans la base
    if (strcmp(kv->mode, "w") == 0) {
        errno = EACCES;
    }
    lseek(kv->fd_kv, LG_EN_TETE_KV, SEEK_SET);
    kv->couple_nr_kv_next = 0;  // initializes the number of read couples to 0
}

int kv_next(KV* kv, kv_datum* key, kv_datum* val) {
    len_t offset_couple;
    int longueur;
    if (strcmp(kv->mode, "w") == 0) {
        errno = EACCES;
        return -1;
    }
    while (getSlotsInDKV(kv) - 1 >= kv->couple_nr_kv_next) {
        // balaie le prochain couple de dkv
        offset_couple = access_offset_dkv(kv->couple_nr_kv_next, kv);
        longueur = access_lg_dkv(kv->couple_nr_kv_next, kv);
        if (longueur >= 0) {
            kv->couple_nr_kv_next++;
            continue;
        } else {
            // remplit d'abord la clef puis recherche la valeur
            if (RemplissageClef(kv, offset_couple, key) == -1)
                return -1;
            if (RemplissageVal(kv, offset_couple, val, key) == -1)
                return -1;
            kv->couple_nr_kv_next++;
            return 1;
        }
    }
    if (getSlotsInDKV(kv) == kv->couple_nr_kv_next)
        return 0;
    return -1;
}

//************************************FIN************************************//

// Accès à dkv

// renvoie la longueur de l'emplacement emplacement
unsigned int access_lg_dkv(unsigned int emplacement, KV* kv) {
    return (*(unsigned int*)(kv->dkv + LG_EN_TETE_DKV + (sizeof(unsigned int) + sizeof(len_t)) * emplacement));
}
// renvoie l'offset de l'emplacement emplacement
len_t access_offset_dkv(int emplacement, KV* kv) {
    unsigned int offset = LG_EN_TETE_DKV + (sizeof(unsigned int) + sizeof(len_t)) * emplacement + sizeof(unsigned int);
    return (*(int*)(kv->dkv + offset));
}

//****************************************************************************//

// Rempli buffbloc en indiquant si à l'offset i+1 il y a un bloc qui est enregistré et si ce bloc est vide ou contient
// des emplacements

int remplit_bloc(KV* kv) {
    int test;
    unsigned char buffbloc[BLOCK_SIZE];
    for (unsigned int i = 0; i < kv->blocIsOccupiedSize; i++) {
        if ((test = readAtPosition(kv->fd_blk, getOffsetBlk(i + 1), buffbloc, BLOCK_SIZE, kv)) == -1)
            return -1;

        if (test == 0)  // there is no block
            kv->blocIsOccupied[i] = false;
        else {
            if (getNbElementsInBlock(buffbloc) == 0)  // No elements in block
                kv->blocIsOccupied[i] = false;
            else
                kv->blocIsOccupied[i] = true;
        }
    }
    return 1;
}

// recherche le numéro de bloc contenu à la position hash du fichier .h
// numéro retourné : numbloc + 1 (vu que les blocs commencent à 1)
// en tête bloc : bloc suivant(char)+nr bloc suivant(int) + nb emplacements (int)
// retourne 0 si aucun bloc alloué, -1 s'il y a une erreur, i>0 si bloc trouvé
int RechercheBlocH(KV* kv, int hash) {
    int numbloc;
    int test;
    int fils;
    unsigned char* bloctmp;

    if ((test = readAtPosition(kv->fd_h, getOffsetH(hash), &numbloc, sizeof(int), kv)) == -1)
        return -1;

    if (test == 0 || numbloc == 0) {  // either read 0 bytes or 0 blocks are written
        return 0;
    } else {
        // Must look till finds a bloc with free entries
        while (1) {
            // Reads the bloc
            if (readAtPosition(kv->fd_blk, getOffsetBlk(numbloc), kv->bloc, BLOCK_SIZE, kv) == -1)
                return -1;

            if ((getNbElementsInBlock(kv->bloc) + 1) * sizeof(len_t) <
                BLOCK_SIZE - LG_EN_TETE_BLOC) {  // Available space
                return numbloc;
            } else if (kv->bloc[0] == 0)  // pas de fils -> en créé un.
            {
                bloctmp = kv->bloc;  // stock temporairement le père ATTENETION SOURCE DE BUGS!!
                fils = Allouebloc(kv);
                if (fils == -1)
                    return -1;
                bloctmp[0] = 1;
                memcpy(bloctmp + 1, &fils, sizeof(unsigned int));

                if (writeAtPosition(kv->fd_blk, getOffsetBlk(numbloc), bloctmp,
                                    getOffsetBloc(getNbElementsInBlock(bloctmp)), kv) == -1)
                    return -1;
                return fils;
            } else {
                numbloc = *(int*)(kv->bloc + 1);
            }
        }
    }
}

// La numérotation des blocs commence à 1
// fonction qui retourne 0 si non trouvé, sinon retourne l'offset de la clef  +1
// retourne l'offset de la clef, 0 si non trouvée ou erreur
len_t RechercheOffsetClef(KV* kv, const kv_datum* key, int numbloc) {
    unsigned char bool_prochain_bloc;
    int test;

    do {
        bool_prochain_bloc = kv->bloc[0];
        for (unsigned int i = 0; i < getNbElementsInBlock(kv->bloc); i++) {
            test = compareClefkv(kv, key, *(int*)(kv->bloc + getOffsetBloc(i)));
            if (test == -1)
                return -1;
            if (test == 1)
                return (*(int*)(kv->bloc + getOffsetBloc(i)) + 1);
        }

        if (bool_prochain_bloc) {
            if ((test = readAtPosition(kv->fd_blk, getOffsetBlk(numbloc), kv->bloc, BLOCK_SIZE, kv)) == -1)
                return 0;
        }
    } while (bool_prochain_bloc);

    errno = EINVAL;  // Key not found
    return 0;
}

// fonction qui retourne 1 si les clefs sont égaux, 0 sinon, -1 si erreur
int compareClefkv(KV* kv, const kv_datum* key, len_t offset) {
    len_t keyLength = 0;  // longueur de la clef
    len_t totalReadBytes = 0;
    int readBytes = 0;
    unsigned char buf[2048];
    unsigned int maxReadBytesInStep = 0;

    // se déplace d'abord au bonne offset dans .kv
    if (readAtPosition(kv->fd_kv, getOffsetKV(offset), &keyLength, sizeof(int), kv) == -1)
        return -1;

    if (keyLength != key->len)
        return 0;
    while (totalReadBytes < keyLength) {
        maxReadBytesInStep = (keyLength - totalReadBytes) % 2049;  // reads at most 2048 bytes at each step
        readBytes = read_controle(kv->fd_kv, buf, maxReadBytesInStep);
        if (readBytes == -1)
            return -1;
        if (memcmp(((unsigned char*)key->ptr) + totalReadBytes, buf, readBytes) != 0)
            return 0;  // part of keys are not equal
        totalReadBytes += (len_t)readBytes;
    }
    return 1;
}

// retourne la taille de la clef ou 0 si erreur
len_t RechercheTailleClef(KV* kv, len_t offset) {
    len_t longueur;
    if (readAtPosition(kv->fd_kv, getOffsetKV(offset), &longueur, sizeof(len_t), kv) == -1)
        return 0;
    return longueur;
}

// Remplit la valeur
// retourne 1 si réussi, -1 sinon
int RemplissageVal(KV* kv, len_t offset, kv_datum* val, const kv_datum* key) {
    if (val == NULL) {
        errno = EINVAL;
        return -1;
    }
    // len_t valueLengthInBytes;
    // unsigned int posVal = getOffsetKV(offset) + sizeof(len_t) + key->len;
    // if (readAtPosition(kv->fd_kv, posVal, &valueLengthInBytes, sizeof(len_t), kv) == -1)
    //     return -1;
    // val->ptr = malloc(valueLengthInBytes);
    // printf("\n\n\nsize val : %d \n\n\n", val->len);

    // if (readAtPosition(kv->fd_kv, posVal + sizeof(len_t), val->ptr, val->len, kv) == -1)
    //     return -1;

    len_t longueur = key->len;

    if (val->ptr == NULL)  // dans ce cas cherche la longueur à allouer
    {
        if (readAtPosition(kv->fd_kv, getOffsetKV(offset), &val->len, sizeof(len_t), kv) == -1)
            return -1;
        val->ptr = malloc(val->len);
    } else  // val->len est le max d'octets que l'on peut lire
    {
        if (readAtPosition(kv->fd_kv, getOffsetKV(offset + longueur + sizeof(len_t)), &longueur, sizeof(unsigned int),
                           kv) == -1)
            return -1;
        // affecte à val->len longueur si longueur<val->len
        val->len = (longueur < val->len) ? longueur : val->len;
    }
    // enregistre dans val->ptr le nb d'octets
    if (read_controle(kv->fd_kv, val->ptr, val->len) == -1) {
        free(val->ptr);
        return -1;
    }
    return 1;
}

// Reads the key stored at position offset into clef
int RemplissageClef(KV* kv, len_t offset, kv_datum* clef) {
    if (clef == NULL) {  // clef must have been allocated
        errno = EINVAL;
        return -1;
    }

    clef->len = RechercheTailleClef(kv, offset);
    if (clef->len == 0)
        return -1;
    clef->ptr = malloc(clef->len);
    if (readAtPosition(kv->fd_kv, getOffsetKV(offset) + sizeof(len_t), clef->ptr, clef->len, kv) == -1)
        return -1;
    return 1;
}

// retourne le nr de bloc vide et l'initialise (suppose qu'il y a assez de place)
// nb_bloc commence à 1
// en-tête de bloc : bloc suivant(char)+nbloc suivant(int) nb emplacements (int)
// MagicN + nb_blocs
// retourne 1 si réussi -1 sinon
int Allouebloc(KV* kv) {
    memset(kv->bloc, 0, BLOCK_SIZE);

    unsigned int i = 0;
    int numbloc = 0;

    while (true) {
        if (i == kv->blocIsOccupiedSize)  // il faut agrandir le tableau
        {
            kv->blocIsOccupiedSize += 100;
            kv->blocIsOccupied = realloc(kv->blocIsOccupied, kv->blocIsOccupiedSize * sizeof(bool));
            memset(kv->blocIsOccupied + kv->blocIsOccupiedSize - 100, 0, 100 * sizeof(bool));  // sets new entries to 0
        }
        if (kv->blocIsOccupied[i] == false)  // le bloc est vide
        {
            numbloc = i;
            kv->blocIsOccupied[i] = true;
            kv->nb_blocs++;
            break;
        }
        i++;  // si pas trouvé regarde au bloc prochain
    }
    return numbloc + 1;  // car la numérotation des blocs commence à 1
}

// écrit le numéro de bloc
// retourne 1 si réussi -1 sinon
int liaisonHBlk(KV* kv, int hash, int numbloc) {
    if (writeAtPosition(kv->fd_h, getOffsetH(hash), &numbloc, sizeof(unsigned int), kv) == -1)
        return -1;
    return 1;
}

// regarde s'il y a encore de la place pour une autre clef + valeur
// et regarde si des voisins sont libres + insérer cet nouvel espace dans dkv
// emplacement_dkv : numéro de l'emplacement dans dkv (i)
void insertionFusionEspace(KV* kv, unsigned int emplacement_dkv) {
    int voisins = -1;  // voisin suivant
    int voisinp = -1;  // voisin précédent
    // recherche l'emplacement suivant et précédent
    if (emplacement_dkv != 0)
        voisinp = emplacement_dkv - 1;
    if (emplacement_dkv + 1 != *(unsigned int*)kv->dkv)
        voisins = emplacement_dkv + 1;
    fusionVoisinsVidesSP(voisins, emplacement_dkv, voisinp, kv);
    // si l'emplacement est le dernier le supprime (diminue le nb d'emplacments
    if (emplacement_dkv == getSlotsInDKV(kv) || emplacement_dkv + 1 == getSlotsInDKV(kv))
        reduceNbEntriesDKV(kv);
}

void reduceNbEntriesDKV(KV* kv) {
    unsigned int nb_emplacements = getSlotsInDKV(kv) - 1;
    memcpy(kv->dkv, &nb_emplacements, sizeof(unsigned int));
}

void fusionVoisinsVidesSP(int leftSlot, int emplacement_dkv, int rightSlot, KV* kv) {
    if (rightSlot != -1)  // si le voisin précédent est vide
    {
        // Updates the offset to the one of voisinp
        unsigned int newOffset = access_offset_dkv(rightSlot, kv);
        memcpy(kv->dkv + getOffsetDkv(emplacement_dkv) + sizeof(len_t), &newOffset, sizeof(unsigned int));
        decaledkv_arriere(kv, rightSlot, emplacement_dkv);  // removes rightSlot
        // removes last element in ftruncate in close
    }
    if (leftSlot != -1)  // si le voisin suivant est vide
    {
        if (rightSlot != -1) {
            leftSlot--;
            emplacement_dkv--;
        }
        decaledkv_arriere(kv, leftSlot, emplacement_dkv);  // removes leftSlot
        // removes last element in ftruncate in close
    }
}

// décalle tout dkv vers une position avant
void decaledkv_arriere(KV* kv, int slotToDelete, int centralSlot) {
    // Adds the length of neighbor to emplacement_dkv
    len_t newSize = access_lg_dkv(centralSlot, kv) + access_lg_dkv(slotToDelete, kv);
    memcpy(kv->dkv + getOffsetDkv(centralSlot), &newSize, sizeof(len_t));

    memcpy(kv->dkv + getOffsetDkv(slotToDelete), kv->dkv + getOffsetDkv(slotToDelete + 1),
           sizeOfDKVFilled(kv) - getOffsetDkv(slotToDelete + 1));
    reduceNbEntriesDKV(kv);
}

// décalle tout dkv vers une position avant
void decaledkv_avant(KV* kv, int emplacement_a_decal) {
    int nb_emplacements = getSlotsInDKV(kv) + 1;
    memcpy(kv->dkv, &nb_emplacements, sizeof(unsigned int));

    if (getSlotsInDKV(kv) == kv->maxElementsInDKV)  // adds space to dkv
        kv = increaseSizeDkv(kv);

    memcpy(kv->dkv + getOffsetDkv(emplacement_a_decal), kv->dkv + getOffsetDkv(emplacement_a_decal + 1),
           sizeOfDKVFilled(kv) - getOffsetDkv(emplacement_a_decal + 1));
}

// libère un emplacement identifié par son offset dans .dkv
void libereEmplacementdkv(len_t offset, KV* kv) {
    unsigned int nb_emplacements = getSlotsInDKV(kv);
    for (unsigned int i = 0; i < nb_emplacements; i++) {
        if (access_offset_dkv(i, kv) == offset) {
            unsigned int newSize = access_lg_dkv(i, kv);
            memcpy(kv->dkv + getOffsetDkv(i), &newSize, sizeof(len_t));
            insertionFusionEspace(kv, i);
            if (i == nb_emplacements)
                reduceNbEntriesDKV(kv);
            return;
        }
    }
}

// Frees an entry in blk. Returns 1 on success and -1 on failure
int libereEmplacementblk(int numbloc, len_t offset, KV* kv, int hash, int previousBloc) {
    unsigned int i;
    do {
        unsigned int nbSlots = getNbElementsInBlock(kv->bloc);
        unsigned int offsetOfLastElementInBloc = *(len_t*)(kv->bloc + getOffsetBloc(nbSlots - 1));
        for (i = 0; i < nbSlots; i++) {
            if (offset == *(len_t*)(kv->bloc + getOffsetBloc(i))) {  // found element
                // swaps the offset of the element with the one of the last element of the bloc
                memcpy(kv->bloc + getOffsetBloc(i), &offsetOfLastElementInBloc, sizeof(unsigned int));
                nbSlots--;
                memcpy(kv->bloc + 5, &nbSlots, sizeof(unsigned int));

                if (nbSlots == 0 && previousBloc == 0 && kv->bloc[0] == 0) {  // frees the bloc entry
                    if (supprimeblocdeh(kv, hash) == -1)
                        return -1;
                    kv->nb_blocs--;
                    kv->blocIsOccupied[numbloc - 1] = false;  // marks the bloc as free
                }

                unsigned int writeBytes = getOffsetBloc(nbSlots);
                if (writeAtPosition(kv->fd_blk, getOffsetBlk(numbloc), kv->bloc, writeBytes, kv) == -1)
                    return -1;
                return 1;
            }
        }
        // entry not found
        previousBloc = numbloc;
        numbloc = (*(int*)(kv->bloc + 1));  // bloc index of next bloc - apparemment assuré que le prochain bloc existe
        if ((readAtPosition(kv->fd_blk, getOffsetBlk(numbloc), kv->bloc, BLOCK_SIZE, kv)) == -1)
            return 0;
    } while (true);
    return 1;
}

// Supprime la référence du bloc dans .h
// retourne -1 en cas d'erreur sinon retourne 1
int supprimeblocdeh(KV* kv, int hash) {
    unsigned int i = 0;
    if (writeAtPosition(kv->fd_h, getOffsetH(hash), &i, sizeof(unsigned int), kv) == -1)
        return -1;
    return 1;
}

// création d'un nouvel emplacement à la fin de dkv
// l'endroit de cet emplacement sera égal à nb_emplacements-1
// en-tête dkv :MagicN+ nb_d'emplacements
void NouvEmplacementDkv(KV* kv, const kv_datum* key, const kv_datum* val) {
    int nb_emplacements = getSlotsInDKV(kv);  // nombre d'emplacements
    len_t offsetNewSlot;

    if (getSlotsInDKV(kv) == kv->maxElementsInDKV)
        kv = increaseSizeDkv(kv);

    len_t lengthNewSlot = sizeof(len_t) + key->len + sizeof(len_t) + val->len;
    memcpy(kv->dkv + getOffsetDkv(nb_emplacements), &lengthNewSlot, sizeof(len_t));
    if (nb_emplacements == 0)
        offsetNewSlot = 0;
    else
        offsetNewSlot = access_offset_dkv(nb_emplacements - 1, kv) + abs(access_lg_dkv(nb_emplacements - 1, kv));
    memcpy(kv->dkv + getOffsetDkv(nb_emplacements) + sizeof(len_t), &offsetNewSlot, sizeof(len_t));
    nb_emplacements++;
    memcpy(kv->dkv, &nb_emplacements, sizeof(unsigned int));
}

// Increases the size of dkv
KV* increaseSizeDkv(KV* database) {
    database->maxElementsInDKV += 1000;
    database->dkv = realloc(database->dkv, sizeOfDKVMax(database));
    return database;
}

// ATTENTION LA NUMEROTATION DES BLOCS COMMENCE A UN DONC FAUT ENLEVER 1 A NUMBLOC
// fonction qui fait le lien entre l'emplacement dans le bloc et kv
int lienBlkKv(int numbloc, KV* kv, len_t emplacement_kv) {
    int bloc = numbloc - 1;  // car la numérotation des blocs commence à 1
    // placement au bon bloc
    // recherche emplacement dans bloc vide
    int nbSlots = getNbElementsInBlock(kv->bloc);
    // indique qu'on a changé de bloc (emplacement_vide contient le num du bloc)
    if (nbSlots == -1)
        return -1;

    memcpy(kv->bloc + getOffsetBloc(nbSlots), &emplacement_kv, sizeof(len_t));
    nbSlots++;  // incrémente nb emplacements
    memcpy(kv->bloc + 5, &nbSlots, sizeof(len_t));

    if (writeAtPosition(kv->fd_blk, getOffsetBlk(bloc + 1), kv->bloc, BLOCK_SIZE, kv) == -1)  // Writes the bloc
        return -1;
    return 1;
}

// met l'emplacement en tant que place occupée dans dkv
KV* SetOccupeDkv(KV* kv, int emplacement_dkv) {
    int length = access_lg_dkv(emplacement_dkv, kv) * (-1);
    memcpy(kv->dkv + getOffsetDkv(emplacement_dkv), &length, sizeof(unsigned int));
    return kv;
}

// voit s'il y a encore de la place pour une autre clef + valeur
// puis insérer cet nouvel espace dans dkv
// emplacement_dkv : numéro de l'emplacement dans dkv (i)
void insertionNewEspace(KV* kv, int emp_dkv, const kv_datum* key, const kv_datum* val) {
    int nbemplacements = getSlotsInDKV(kv);
    len_t taille_couple = sizeof(len_t) + key->len + sizeof(len_t) + val->len;
    int octets_restants = (-1) * access_lg_dkv(emp_dkv, kv) - (taille_couple);
    // regarde si place pour une autre clef
    if (octets_restants - 10 > 0)  // s'il reste plus de 10 octets (2int + 2 octets)
    {
        int tmp = taille_couple * (-1);
        memcpy(kv->dkv + getOffsetDkv(emp_dkv), &tmp, sizeof(unsigned int));
    } else  // sinon retourne kv
        return;

    if (emp_dkv == nbemplacements - 1)
        return;
    // si n'a pas pu fusionner avec le voisin précédent, fait un nouvel emplct
    creationNewVoisin(kv, octets_restants, emp_dkv + 1, taille_couple);
}

// créé un nouvel emplacement
void creationNewVoisin(KV* kv, int octets_restants, int empl_dkv, len_t requiredSize) {
    decaledkv_avant(kv, empl_dkv);  // décalle d'abord tout dkv vers une position à l'avant
    unsigned offsetNewSlot = access_offset_dkv(empl_dkv - 1, kv) + requiredSize;
    memcpy(kv->dkv + getOffsetDkv(empl_dkv), &octets_restants, sizeof(len_t));
    memcpy(kv->dkv + getOffsetDkv(empl_dkv) + sizeof(len_t), &offsetNewSlot, sizeof(unsigned int));
}

// tronque kv
int truncate_kv(KV* kv) {
    if (strcmp(kv->mode, "r") == 0)  // pas besoin de tronquer vu que rien n'a changé
        return 1;
    len_t offset_max = 0;
    len_t longueur_max = 0;
    unsigned int nbElementsInDKV = getSlotsInDKV(kv);
    for (unsigned int i = 0; i < nbElementsInDKV; i++) {
        if (offset_max <= access_offset_dkv(i, kv)) {
            offset_max = access_offset_dkv(i, kv);
            longueur_max = abs(access_lg_dkv(i, kv));
        }
    }
    // on tronque à offset_max+longueur_max + LG_EN_TETE_KV
    if (ftruncate(kv->fd_kv, offset_max + longueur_max + LG_EN_TETE_KV) == -1)
        return -1;
    return 1;
}
