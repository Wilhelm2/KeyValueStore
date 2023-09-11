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
    int test_existance;
    len_t nb_emplacements_dkv;
    unsigned char* buf_dkv;
    int (*f_hachage)(const kv_datum* kdatum);

    if ((test_existance = initializeFileDescriptors(kv, mode, dbname)) == -1)
        return NULL;

    // base de donnée n'existe pas ou mode w ou w+
    if ((strcmp(mode, "w") == 0) || (strcmp(mode, "w+") == 0) || test_existance == 0) {
        // Ajout de la fonction de hachage dans l'en-tête de dbname.h
        write_controle(kv->descr_h, &hidx, 4);
        // se place avant la fin de l'en-tête pour écrire 0 mettre le nb d'empla
        // cements de dkv à 0
        int j = 0;
        if (writeAtPosition(kv->descr_dkv, 1, &j, 4, kv) == -1)
            return NULL;

        if (writeAtPosition(kv->descr_blk, 1, &j, 4, kv) == -1)
            return NULL;
    }

    // Reads the number of slots in dkv
    if (readAtPosition(kv->descr_dkv, LG_EN_TETE_DKV - 4, &nb_emplacements_dkv, sizeof(int), kv) == -1)
        return NULL;

    if (readAtPosition(kv->descr_blk, 1, &kv->nb_blocs, 4, kv) == -1)
        return NULL;

    // ajout de 1000 octets supplémentaires pour ajouter de nouveaux couples
    buf_dkv = malloc(nb_emplacements_dkv * 8 + 1000 + 4 + 8);
    // ajout de 40 places pour ajouter de nouveaux couples
    kv->tabbloc = malloc(sizeof(int) * (40 + kv->nb_blocs));

    // se déplace en arrière pour également lire le nb d'emplacements
    if (readAtPosition(kv->descr_dkv, LG_EN_TETE_DKV - 4, buf_dkv, nb_emplacements_dkv * 4 + 4, kv) == -1)
        return NULL;

    int indexbase;
    if (readAtPosition(kv->descr_h, 1, &indexbase, 4, kv) == -1)
        return NULL;

    f_hachage = choix_hachage(indexbase);
    if (f_hachage == NULL)  // vérifie qu'il existe une fonction associée à index
    {
        errno = EINVAL;
        return NULL;
    }

    kv->longueur_buf_bloc = 40 + kv->nb_blocs;
    memset(kv->tabbloc, 0, kv->longueur_buf_bloc * sizeof(int));
    // fait un read de chaque position pour voir si le bloc est déjà occupé ou non
    // tabbloc = 0 si vide et 1 si occupé
    if (remplit_bloc(0, kv) == -1) {
        kv_close(kv);
        return NULL;
    }

    // On enregistre tout dans KV;
    kv->methode_alloc = alloc;
    kv->dkv = buf_dkv;
    kv->remplissement_dkv = (len_t)nb_emplacements_dkv * 8 + LG_EN_TETE_DKV - 1;
    kv->longueur_dkv = (len_t)(4 + nb_emplacements_dkv * 8 + 1000);
    kv->mode = mode;
    kv->f_hachage = f_hachage;
    kv->bloc = NULL;
    return kv;
}

// ferme la base de donnée, retourne -1 si échoue, sinon retourne 0
int kv_close(KV* kv) {
    int res = 0;

    if (strcmp(kv->mode, "r") != 0) {
        // écrture de dkv
        if (writeAtPosition(kv->descr_dkv, 1, kv->dkv, kv->remplissement_dkv, kv) == -1) {
            freeDatabase(kv);
            return -1;
        }

        if (writeAtPosition(kv->descr_blk, 1, &kv->nb_blocs, 4, kv) == -1) {
            freeDatabase(kv);
            return -1;
        }

        // met .dkv à la bonne taille : LG_EN_TETE_DKV + nb_emplacements*8
        // pas besoin de fstat vu qu'on a toutes les données
        if (ftruncate(kv->descr_dkv, kv->remplissement_dkv + 1) == -1) {
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
    free(database->tabbloc);
    free(database);
}

// fonction qui recherche la valeur associée a key
// Retour : 1 trouvé, 0 non trouvé, -1 erreur
int kv_get(KV* kv, const kv_datum* key, kv_datum* val) {
    int hash = kv->f_hachage(key);
    int numbloc;
    len_t offset;
    if (strcmp(kv->mode, "w") == 0) {  // not allowed to read
        errno = EACCES;
        return -1;
    }

    if ((numbloc = RechercheBlocH(kv, hash)) < 1) {  // either no bloc associated  to that hashor error
        if (numbloc == 0)                            // no bloc
            errno = EINVAL;
        return numbloc;
    }

    // recherche de la clef dans .kv, retourne l'offset si trouvé, -1 sinon
    offset = RechercheOffsetClef(kv, key, numbloc);
    if (offset == 0) {
        free(kv->bloc);
        return 0;
    } else
        offset--;  // enlève de nouveau le +1 ajouté dans RechercheOffsetClef
    if (RemplissageVal(kv, offset, val, key) == -1) {
        free(kv->bloc);
        return -1;
    }
    free(kv->bloc);
    if (val == NULL)
        return -1;
    return 1;
}

int kv_put(KV* kv, const kv_datum* key, const kv_datum* val) {
    int hash = kv->f_hachage(key);
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
        free(kv->bloc);
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

    if (writeAtPosition(kv->descr_kv, getOffsetKV(offset), tabwrite, nbBytesToWrite, kv) == -1)
        return -1;

    free(tabwrite);
    return 1;
}

int kv_del(KV* kv, const kv_datum* key) {
    int hash = kv->f_hachage(key);
    int numbloc;
    len_t offset;
    int test;
    // vérifie que la base a été ouverte avec un mode autorisé
    if (strcmp(kv->mode, "r") == 0) {
        errno = EACCES;
        return -1;
    }

    if ((test = readAtPosition(kv->descr_h, getOffsetH(hash), &numbloc, sizeof(int), kv)) == -1)
        return -1;

    // si la fonction retourne 0 alors il n'y a pas de bloc avec cette valeur
    if (test == 0 || numbloc == 0) {
        errno = ENOENT;
        return -1;
    }

    // lit le bloc
    kv->bloc = malloc(4096);
    if (readAtPosition(kv->descr_blk, getOffsetBlk(numbloc), kv->bloc, 4096, kv) == -1)
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
    lseek(kv->descr_kv, LG_EN_TETE_KV, SEEK_SET);
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
    return (*(unsigned int*)(kv->dkv + 4 + 8 * emplacement));
}
// renvoie l'offset de l'emplacement emplacement
len_t access_offset_dkv(int emplacement, KV* kv) {
    return (*(int*)(kv->dkv + 4 + 8 * emplacement + 4));
}

//****************************************************************************//

// prend comme argument le numéro de bloc (commençant par 1)
// retourne 1 si réussi -1 sinon
int remplit_bloc(int debut, KV* kv) {
    int test;
    unsigned char buffbloc[4096];
    for (unsigned int i = debut; i < kv->longueur_buf_bloc; i++) {
        if ((test = readAtPosition(kv->descr_blk, getOffsetBlk(i + 1), buffbloc, 4096, kv)) == -1)
            return -1;

        if (test == 0)  // il n'y a aucun bloc
        {
            kv->tabbloc[i] = 0;  // indique que le bloc est vide
        } else {
            // regarde si le nombre d'emplacements est vide
            if (getNbElementsInBlock(buffbloc) == 0)
                kv->tabbloc[i] = 0;  // indique que le bloc est vide
            else
                kv->tabbloc[i] = 1;  // indique que le bloc est occupé
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

    if ((test = readAtPosition(kv->descr_h, getOffsetH(hash), &numbloc, sizeof(int), kv)) == -1)
        return -1;

    if (test == 0 || numbloc == 0) {  // either read 0 bytes or 0 blocks are written
        return 0;
    } else {
        // dans ce cas il faut encore regarder si le bloc est vide,
        // puis si son fils est vide, etc... Jusqu'à trouver un emplacement vide
        kv->bloc = malloc(4096);
        while (1) {
            if (readAtPosition(kv->descr_blk, getOffsetBlk(numbloc), kv->bloc, 4096, kv) == -1)
                return -1;

            if ((getNbElementsInBlock(kv->bloc) + 1) * sizeof(len_t) < 4096 - LG_EN_TETE_BLOC) {  // Available space
                return numbloc;
            } else if (kv->bloc[0] == 0)  // pas de fils -> en créé un.
            {
                bloctmp = kv->bloc;  // stock temporairement le père
                fils = Allouebloc(kv);
                if (fils == -1) {
                    free(kv->bloc);
                    return -1;
                }
                bloctmp[0] = 1;
                memcpy(bloctmp + 1, &fils, sizeof(unsigned int));

                if (writeAtPosition(kv->descr_blk, getOffsetBlk(numbloc), bloctmp,
                                    getOffsetBloc(getNbElementsInBlock(bloctmp)), kv) == -1)
                    return -1;

                free(bloctmp);
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
    unsigned char bool_prochain_bloc = kv->bloc[0];
    int test;
    if (numbloc < 0)  // on a changé de bloc -> faut mettre à jour
    {
        numbloc *= -1;
        if ((test = readAtPosition(kv->descr_blk, getOffsetBlk(numbloc), kv->bloc, 4096, kv)) == -1)
            return 0;
    }
    for (unsigned int i = 0; i < getNbElementsInBlock(kv->bloc);
         i++) {  // passe tous les emplacements occupés dans le bloc
        test = compareClefkv(kv, key, *(int*)(kv->bloc + getOffsetBloc(i)));
        if (test == -1) {
            errno = EINVAL;
            return 0;
        }
        if (test == 1)
            return (*(int*)(kv->bloc + getOffsetBloc(i)) + 1);
    }
    // sinon regarde s'il y a un prochain bloc et si oui rappel la fonction
    if (bool_prochain_bloc == 1)
        return (RechercheOffsetClef(kv, key, (*(int*)(kv->bloc + 1)) * (-1)));
    // sinon retourne 0
    errno = EINVAL;
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
    if (readAtPosition(kv->descr_kv, getOffsetKV(offset), &keyLength, sizeof(int), kv) == -1)
        return -1;

    if (keyLength != key->len)
        return 0;
    while (totalReadBytes < keyLength) {
        maxReadBytesInStep = (keyLength - totalReadBytes) % 2049;  // reads at most 2048 bytes at each step
        readBytes = read_controle(kv->descr_kv, buf, maxReadBytesInStep);
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
    if (readAtPosition(kv->descr_kv, getOffsetKV(offset), &longueur, sizeof(len_t), kv) == -1)
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
    // if (readAtPosition(kv->descr_kv, posVal, &valueLengthInBytes, sizeof(len_t), kv) == -1)
    //     return -1;
    // val->ptr = malloc(valueLengthInBytes);
    // printf("\n\n\nsize val : %d \n\n\n", val->len);

    // if (readAtPosition(kv->descr_kv, posVal + sizeof(len_t), val->ptr, val->len, kv) == -1)
    //     return -1;

    len_t longueur = key->len;

    if (val->ptr == NULL)  // dans ce cas cherche la longueur à allouer
    {
        if (readAtPosition(kv->descr_kv, getOffsetKV(offset), &val->len, sizeof(len_t), kv) == -1)
            return -1;
        val->ptr = malloc(val->len);
    } else  // val->len est le max d'octets que l'on peut lire
    {
        if (readAtPosition(kv->descr_kv, getOffsetKV(offset + longueur + sizeof(len_t)), &longueur,
                           sizeof(unsigned int), kv) == -1)
            return -1;
        // affecte à val->len longueur si longueur<val->len
        val->len = (longueur < val->len) ? longueur : val->len;
    }
    // enregistre dans val->ptr le nb d'octets
    if (read_controle(kv->descr_kv, val->ptr, val->len) == -1) {
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
    if (readAtPosition(kv->descr_kv, getOffsetKV(offset) + sizeof(len_t), clef->ptr, clef->len, kv) == -1)
        return -1;
    return 1;
}

// retourne le nr de bloc vide et l'initialise (suppose qu'il y a assez de place)
// nb_bloc commence à 1
// en-tête de bloc : bloc suivant(char)+nbloc suivant(int) nb emplacements (int)
// MagicN + nb_blocs
// retourne 1 si réussi -1 sinon
int Allouebloc(KV* kv) {
    kv->bloc = malloc(4096);
    unsigned int i = 0;
    int vide = 1;
    int numbloc = 0;
    int* nouvtab;

    while (vide == 1) {
        if (i == kv->longueur_buf_bloc)  // il faut agrandir le tableau
        {
            nouvtab = calloc(kv->longueur_buf_bloc + 100, sizeof(int));
            memcpy(nouvtab, kv->tabbloc, kv->longueur_buf_bloc);
            kv->longueur_buf_bloc += 100;
            free(kv->tabbloc);
            kv->tabbloc = nouvtab;
            // remplit tabbloc (regarde dans blk si les blocs sont occupés )
            if (remplit_bloc(kv->longueur_buf_bloc - 100, kv) == -1)
                return -1;
        }
        if (kv->tabbloc[i] == 0)  // le bloc est vide
        {
            numbloc = i;
            kv->tabbloc[i] = 1;
            memset(kv->bloc, 0, 10);
            vide = 0;  // on a trouvé un bloc vide -> sort du while
            kv->nb_blocs++;
        }
        i++;  // si pas trouvé regarde au bloc prochain
    }
    return numbloc + 1;  // car la numérotation des blocs commence à 1
}

// écrit le numéro de bloc
// retourne 1 si réussi -1 sinon
int liaisonHBlk(KV* kv, int hash, int numbloc) {
    if (writeAtPosition(kv->descr_h, getOffsetH(hash), &numbloc, sizeof(unsigned int), kv) == -1)
        return -1;
    return 1;
}

// regarde s'il y a encore de la place pour une autre clef + valeur
// et regarde si des voisins sont libres + insérer cet nouvel espace dans dkv
// emplacement_dkv : numéro de l'emplacement dans dkv (i)
void insertionFusionEspace(KV* kv, int emplacement_dkv) {
    int voisins = -1;  // voisin suivant
    int voisinp = -1;  // voisin précédent
    // recherche l'emplacement suivant et précédent
    if (emplacement_dkv != 0)
        voisinp = emplacement_dkv - 1;
    if (emplacement_dkv + 1 != *(int*)kv->dkv)
        voisins = emplacement_dkv + 1;
    fusionVoisinsVidesSP(voisins, emplacement_dkv, voisinp, kv);
    // si l'emplacement est le dernier le supprime (diminue le nb d'emplacments
    if (emplacement_dkv == getSlotsInDKV(kv) || emplacement_dkv + 1 == getSlotsInDKV(kv))
        reduceNbEntriesDKV(kv);
}

void reduceNbEntriesDKV(KV* kv) {
    unsigned int nb_emplacements = getSlotsInDKV(kv) - 1;
    memcpy(kv->dkv, &nb_emplacements, sizeof(unsigned int));
    kv->remplissement_dkv -= 8;
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
           kv->longueur_dkv - getOffsetDkv(slotToDelete + 1));
    reduceNbEntriesDKV(kv);
}

// décalle tout dkv vers une position avant
void decaledkv_avant(KV* kv, int emplacement_a_decal) {
    int nb_emplacements = getSlotsInDKV(kv) + 1;
    memcpy(kv->dkv, &nb_emplacements, sizeof(unsigned int));
    kv->remplissement_dkv += 8;

    if (kv->longueur_dkv < kv->remplissement_dkv)  // dkv has not enough space
        kv = dkv_aggrandissement(kv);

    memcpy(kv->dkv + getOffsetDkv(emplacement_a_decal), kv->dkv + getOffsetDkv(emplacement_a_decal + 1),
           kv->longueur_dkv - getOffsetDkv(emplacement_a_decal + 1));
}

// libère un emplacement identifié par son offset dans .dkv
void libereEmplacementdkv(len_t offset, KV* kv) {
    unsigned int nb_emplacements = getSlotsInDKV(kv);
    for (unsigned int i = 0; i < nb_emplacements; i++) {
        // saute  nb_emplacements + i*(1+lg) + i*offset
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

// libère un emplacement dans blk
// en-tête bloc:  bloc suivant(char)+nr bloc suivant(int) + nb emplacements (int)
// bloc pere vaut 0 s'il y en a pas, le num du bloc père sinon
// retourne 1 si réussi, -1 sinon
int libereEmplacementblk(int numbloc, len_t offset, KV* kv, int hash, int bloc_p) {
    unsigned int nb_emplacements, i;
    len_t offset_d_emplct;
    // enregistre le nombre d'emplacements
    nb_emplacements = getNbElementsInBlock(kv->bloc);
    // enregistre l'offset du dernier emplacement
    offset_d_emplct = *(len_t*)(kv->bloc + getOffsetBloc(nb_emplacements - 1));

    // recherche d'abord l'emplacement à supprimer
    for (i = 0; i < nb_emplacements; i++) {
        // Si on a trouvé le bon emplacement
        if (offset == *(len_t*)(kv->bloc +
                                getOffsetBloc(i))) {  // dans ce cas on met l'offset du dernier emplacement à la place i
            // remplace l'offset contenu dans i par l'offset du dernier emplct
            memcpy(kv->bloc + getOffsetBloc(i), &offset_d_emplct, sizeof(unsigned int));
            unsigned int tmp = nb_emplacements - 1;
            memcpy(kv->bloc + 5, &tmp, sizeof(unsigned int));  // décrémente le nb d'emplacements
            break;                                             // emplacement trouvé donc sort de for
        }
    }
    // indique que l'emplacement a pas été trouvé
    if (i == nb_emplacements || nb_emplacements == 0) {
        free(kv->bloc);
        // rééexcute la fonction avec le bloc prochain(c'est sûr qu'il y en a un)
        //-> pour savoir s'il y en a un on le * -1 (car il faudra le lire)
        return libereEmplacementblk((-1) * (*(int*)(kv->bloc + 1)), offset, kv, hash, numbloc);
    } else {
        // dans ce cas on libère le bloc dans .h
        if (nb_emplacements - 1 == 0 && bloc_p == 0 && kv->bloc[0] == 0) {
            if (supprimeblocdeh(kv, hash) == -1)
                return -1;
            kv->nb_blocs--;
            kv->tabbloc[numbloc - 1] = 0;  // libère le bloc
        }

        unsigned int writeBytes = getOffsetBloc(nb_emplacements - 1);
        if (writeAtPosition(kv->descr_blk, getOffsetBlk(numbloc), kv->bloc, writeBytes, kv) == -1) {
            free(kv->bloc);
            return -1;
        }
        free(kv->bloc);
    }
    return 1;
}

// Supprime la référence du bloc dans .h
// retourne -1 en cas d'erreur sinon retourne 1
int supprimeblocdeh(KV* kv, int hash) {
    unsigned int i = 0;
    if (writeAtPosition(kv->descr_h, getOffsetH(hash), &i, sizeof(unsigned int), kv) == -1)
        return -1;
    return 1;
}

// création d'un nouvel emplacement à la fin de dkv
// l'endroit de cet emplacement sera égal à nb_emplacements-1
// en-tête dkv :MagicN+ nb_d'emplacements
void NouvEmplacementDkv(KV* kv, const kv_datum* key, const kv_datum* val) {
    int nb_emplacements = getSlotsInDKV(kv);  // nombre d'emplacements
    len_t offsetNewSlot;

    kv->remplissement_dkv += 8;                    // incrémente le remplissement de dkv
    if (kv->longueur_dkv < kv->remplissement_dkv)  // le buffer n'est pas assez grand
        kv = dkv_aggrandissement(kv);              // réalloue de la place : aggrandit le buffer

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

// fonction qui agrandit dkv
KV* dkv_aggrandissement(KV* kv) {
    kv->longueur_dkv += 1000;
    kv->dkv = copie_tableau(kv->dkv, kv->remplissement_dkv);
    return kv;
}
unsigned char* copie_tableau(unsigned char* tableau, unsigned int longueur) {
    unsigned char* dest = malloc(longueur + 1000);
    memcpy(dest, tableau, longueur);
    free(tableau);
    return dest;
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

    if (writeAtPosition(kv->descr_blk, getOffsetBlk(bloc + 1), kv->bloc, 4096, kv) == -1)  // Writes the bloc
        return -1;
    free(kv->bloc);
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
    len_t taille_couple = 4 + key->len + 4 + val->len;
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
    unsigned int nb_emplacements_dkv = getSlotsInDKV(kv);
    for (unsigned int i = 0; i < nb_emplacements_dkv; i++) {
        if (offset_max <= access_offset_dkv(i, kv)) {
            offset_max = access_offset_dkv(i, kv);
            longueur_max = abs(access_lg_dkv(i, kv));
        }
    }
    // on tronque à offset_max+longueur_max + LG_EN_TETE_KV
    if (ftruncate(kv->descr_kv, offset_max + longueur_max + LG_EN_TETE_KV) == -1)
        return -1;
    return 1;
}
