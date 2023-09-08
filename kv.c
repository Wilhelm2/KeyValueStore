// WILHELM Daniel YAMAN Kendal

#include "kv.h"
/*r : O_RDONLY
r+ : O_CREAT|O_RDWR
w : O_CREAT|O_TRUNC|O_WRONLY
w+ : O_CREAT|O_RDWR
*/
// retourne NULL si n'arrive pas à ouvrir la base de donnée
KV* kv_open(const char* dbname, const char* mode, int hidx, alloc_t alloc) {
    KV* kv = malloc(sizeof(struct KV));
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
        if (writeAtPosition(kv->descr_dkv, LG_EN_TETE_DKV - 4, &j, 4, kv) == -1)
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
    memset(kv->tabbloc, 0, (40 + kv->nb_blocs) * sizeof(int));
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

int readAtPosition(int fd, unsigned int position, void* dest, unsigned int nbBytes, KV* database) {
    if (lseek(fd, position, SEEK_SET) == -1) {
        closeFileDescriptors(database);
        free(database);
        return -1;
    }

    // lecture de dkv en mémoire
    if (read_controle(fd, dest, nbBytes) == -1) {
        closeFileDescriptors(database);
        free(database);
        return -1;
    }
    return 1;
}

int writeAtPosition(int fd, unsigned int position, void* src, unsigned int nbBytes, KV* database) {
    if (lseek(fd, position, SEEK_SET) == -1) {
        closeFileDescriptors(database);
        free(database);
        return -1;
    }

    // lecture de dkv en mémoire
    if (write_controle(fd, src, nbBytes) == -1) {
        closeFileDescriptors(database);
        free(database);
        return -1;
    }
    return 1;
}

int initializeFileDescriptors(KV* database, const char* mode, const char* dbname) {
    int flag = getOpenMode(mode);
    int databaseExits;

    char* dbnh = concat(dbname, ".h");
    if (open(dbnh, O_RDONLY, 0666) == -1) {
        // Database doesn't exist and tried to open it in read mode or other reason than not existing database
        if (errno != ENOENT || strcmp(mode, "r") == 0) {
            free(dbnh);
            free(database);
            return -1;
        }
        databaseExits = 0;  // database does not exist
    } else
        databaseExits = 1;  // database exists

    database->descr_h = tryOpen(database, dbnh, flag, 0666);
    free(dbnh);
    if (database->descr_h == -1)
        return -1;

    char* dbnblk = concat(dbname, ".blk");
    database->descr_blk = tryOpen(database, dbnblk, flag, 0666);
    free(dbnblk);
    if (database->descr_blk == -1)
        return -1;

    char* dbnkv = concat(dbname, ".kv");
    database->descr_kv = tryOpen(database, dbnkv, flag, 0666);
    free(dbnkv);
    if (database->descr_kv == -1)
        return -1;

    char* dbndkv = concat(dbname, ".dkv");
    database->descr_dkv = tryOpen(database, dbndkv, flag, 0666);
    free(dbndkv);
    if (database->descr_dkv == -1)
        return -1;

    // Controls that all files have the right magic number
    if (verificationMagicN(database->descr_h, mode, '1', databaseExits) != 1 ||
        verificationMagicN(database->descr_blk, mode, '2', databaseExits) != 1 ||
        verificationMagicN(database->descr_kv, mode, '3', databaseExits) != 1 ||
        verificationMagicN(database->descr_dkv, mode, '4', databaseExits) != 1) {
        closeFileDescriptors(database);
        free(database);
        return -1;
    }

    return databaseExits;
}

// Get opening mode of file following mode. Opening modes are: r,r+,w,w+
int getOpenMode(const char* mode) {
    int flag;
    if (strcmp(mode, "r") == 0)
        flag = O_RDONLY;
    if (strcmp(mode, "r+") == 0)
        flag = O_CREAT | O_RDWR;
    if (strcmp(mode, "w") == 0)
        flag = O_CREAT | O_TRUNC | O_RDWR;
    if (strcmp(mode, "w+") == 0)
        flag = O_CREAT | O_TRUNC | O_RDWR;
    return flag;
}

// Close either returns 0 when succeeds or -1 when fails. Thus the function will return -1 if one of the closes fail
int closeFileDescriptors(KV* database) {
    int res = 0;
    if (database->descr_blk > 0)
        res |= close(database->descr_blk);
    if (database->descr_dkv > 0)
        res |= close(database->descr_dkv);
    if (database->descr_h > 0)
        res |= close(database->descr_h);
    if (database->descr_kv > 0)
        res |= close(database->descr_kv);
    return res;
}

// Verifies that files contain the right magic numbers
int verificationMagicN(int descr, const char* mode, unsigned char magicNumber, int databaseExists) {
    unsigned char buf;
    if (strcmp(mode, "w") == 0 || strcmp(mode, "w+") == 0 || !databaseExists) {
        // No verification required because database truncated or just created
        write_controle(descr, &magicNumber, 1);
    } else if ((strcmp(mode, "r") == 0 || strcmp(mode, "r+") == 0) && databaseExists) {
        // Verifies that file contains right magic number
        if (read_controle(descr, &buf, 1) == -1)
            return -1;
        if (buf != magicNumber)
            return 0;
    }
    return 1;
}

int tryOpen(KV* database, const char* filename, int flags, unsigned int mode) {
    int fd = open(filename, flags, mode);
    if (fd == -1) {
        closeFileDescriptors(database);
        free(database);
    }
    return fd;
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
    int hache;
    int numbloc;
    len_t offset;
    // vérifie que la base a été ouverte avec un mode autorisé
    if (strcmp(kv->mode, "w") == 0) {
        errno = EACCES;
        return -1;  // erreur
    }
    hache = kv->f_hachage(key);

    // appel de la fonction qui vérifie s'il y a un bloc qui existe à cette position
    // si la fonction retourne 0 alors il n'y a pas de bloc avec cette valeur
    if ((numbloc = RechercheBlocH(kv, hache)) == 0) {
        errno = EINVAL;
        return 0;
    }
    if (numbloc == -1) {
        return -1;
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
    int (*f_allocation)(KV * kv, const kv_datum* key, const kv_datum* val);
    int hache;
    int numbloc;
    int emplacement_dkv;
    len_t offset;
    unsigned char* tabwrite;
    int test;
    // vérifie que la base a été ouverte avec un mode autorisé
    if (strcmp(kv->mode, "r") == 0) {
        errno = EACCES;
        return -1;
    }

    // calcul le hache de la clef
    hache = kv->f_hachage(key);

    // fonction qui vérifie s'il y a un bloc qui existe à cette position
    // si la fonction retourne 0 alors il n'y a pas de bloc avec cette valeur
    // donc il faut en allouer un
    if ((numbloc = RechercheBlocH(kv, hache)) == 0) {
        // alloue un bloc initialisé puis retourne
        // le numéro de ce bloc (la numéroation débute à 1)
        numbloc = Allouebloc(kv);
        if (numbloc == -1)
            return -1;
        if (liaisonHBlk(kv, hache, numbloc) == -1)  // liaison entre .h et .blk
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
        if ((numbloc = RechercheBlocH(kv, hache)) == 0) {
            numbloc = Allouebloc(kv);  // realloue un bloc
            if (numbloc == -1)
                return -1;
            if (liaisonHBlk(kv, hache, numbloc) == -1)  // liaison entre .h et .blk
                return -1;
        }
        if (numbloc == -1)
            return -1;
    }

    // choix de la bonne méthode d'allocation dans .kv
    f_allocation = choix_allocation(kv);

    // recherche d'un emplacement pour stocker la valeur
    if ((emplacement_dkv = f_allocation(kv, key, val)) == -1) {  // créé un nouvel emplacement dans dkv
        NouvEmplacementDkv(kv, key, val);
        emplacement_dkv = *(int*)(kv->dkv) - 1;
    }

    offset = access_offset_dkv(emplacement_dkv, kv);
    // enregistrement de la clef à l'offset
    if (lseek(kv->descr_kv, LG_EN_TETE_KV + offset, SEEK_SET) == -1) {
        perror("lseek");
        exit(1);
    }
    // écriture du couple
    // on rentre d'abord tout dans un tableau puis on fait un seul write
    tabwrite = malloc((4 + key->len + 4 + val->len) * sizeof(unsigned char));
    tabwrite = modif(tabwrite, key->len, 0);  // écriture de la longueur de clef

    memcpy(tabwrite + sizeof(len_t), key->ptr, key->len);

    tabwrite = modif(tabwrite, val->len, 4 + key->len);
    memcpy(tabwrite + sizeof(len_t) + key->len + sizeof(len_t), val->ptr, val->len);

    if (write_controle(kv->descr_kv, tabwrite, 4 + key->len + 4 + val->len) == -1) {
        free(tabwrite);
        return -1;
    }
    free(tabwrite);

    // établissement de la liaison entre .blk et .kv
    if (lienBlkKv(numbloc, kv, offset) == -1)
        return -1;

    // met l'emplacement en tant que place occupée dans dkv
    kv = SetOccupeDkv(kv, emplacement_dkv);
    // voit s'il y a encore de la place pour un autre couple et l'insère
    insertionNewEspace(kv, emplacement_dkv, key, val);
    return 0;
}

int kv_del(KV* kv, const kv_datum* key) {
    int hache;
    int numbloc;
    len_t offset;
    int test;
    // vérifie que la base a été ouverte avec un mode autorisé
    if (strcmp(kv->mode, "r") == 0) {
        errno = EACCES;
        return -1;
    }

    // calcul du hache
    hache = kv->f_hachage(key);

    if (lseek(kv->descr_h, LG_EN_TETE_H + hache * 4, SEEK_SET) == -1)
        return -1;

    // regarde si un bloc est alloué
    if ((test = read(kv->descr_h, &numbloc, 4)) == -1)
        return -1;

    // si la fonction retourne 0 alors il n'y a pas de bloc avec cette valeur
    if (test == 0 || numbloc == 0) {
        errno = ENOENT;
        return -1;
    }

    // lit le bloc
    kv->bloc = malloc(4096);
    if (lseek(kv->descr_blk, LG_EN_TETE_BLK + 4096 * (numbloc - 1), SEEK_SET) == -1)
        return -1;
    if ((read(kv->descr_blk, kv->bloc, 4096)) == -1)
        return -1;

    offset = RechercheOffsetClef(kv, key, numbloc);
    if (offset == 0 && errno != EINVAL)  // erreur
        return -1;
    if (offset == 0)  // clef non contenue
    {
        errno = ENOENT;
        return -1;
    }
    offset--;  // corrige l'offset renvoyé par RechercheOffsetClef

    // la clef est contenue dans kv et son offset est offset, on la supprime
    libereEmplacementdkv(offset, kv);
    if (libereEmplacementblk(numbloc, offset, kv, hache, 0) == -1)
        return -1;
    return 0;
}
void kv_start(KV* kv) {
    // si mode = w alors pas le droit de lire dans la base
    if (strcmp(kv->mode, "w") == 0) {
        errno = EACCES;
    }
    lseek(kv->descr_kv, LG_EN_TETE_KV, SEEK_SET);
    // initialise le nombre de couple lus de kv à 0
    kv->couple_nr_kv_next = 0;
    return;
}

int kv_next(KV* kv, kv_datum* key, kv_datum* val) {
    len_t offset_couple;
    int longueur;
    if (strcmp(kv->mode, "w") == 0) {
        errno = EACCES;
        return -1;
    }
    while (*(int*)kv->dkv - 1 >= kv->couple_nr_kv_next) {
        // balaie le prochain couple de dkv
        offset_couple = *(int*)(kv->dkv + 4 + kv->couple_nr_kv_next * 8 + 4);
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
    if (*(int*)kv->dkv == kv->couple_nr_kv_next)
        return 0;
    return -1;
}

//************************************FIN************************************//

// Fonctions générales de read et write
int read_controle(int descripteur, void* ptr, int nboctets) {
    int test;
    if ((test = read(descripteur, ptr, nboctets)) == -1)
        return -1;
    if (test != nboctets) {
        errno = EINVAL;
        return -1;
    }
    return test;
}
int write_controle(int descripteur, const void* ptr, int nboctets) {
    int test;
    if ((test = write(descripteur, ptr, nboctets)) == -1)
        return -1;
    if (test != nboctets) {
        errno = EINVAL;
        return -1;
    }
    return nboctets;
}

//****************************************************************************//

// Fonctions de hachage

// fonction qui retourne un pointeur sur une fonction de hachage en fonction
// de l'index. Retourne -1 si aucune f de hachage correspond à l'index
int (*choix_hachage(int index))(const kv_datum* clef) {
    if (index == 1)
        return hachage_test;
    if (index == 0)
        return hachage_defaut;
    if (index == 2)
        return djb_hash;
    if (index == 3)
        return fnv_hash;
    else
        return NULL;
}

// Fonction de hachage par défaut
int hachage_defaut(const kv_datum* clef) {
    unsigned char* ptrclef = clef->ptr;
    unsigned int i, s = 0;
    for (i = 0; i < clef->len; i++) {
        s += ptrclef[i] % 999983;
    }
    return (s);
}

// hachage de test -> renvoie toujours le même hache ->
// test si l'allocation d'un prochain bloc fonctionne -> retourne toujours 0
int hachage_test(const kv_datum* clef) {
    return clef->len % 1;
}

// Bernstein hash
int djb_hash(const kv_datum* clef) {
    unsigned int hash = 5381, i;
    unsigned char* ptrclef = (unsigned char*)clef->ptr;

    for (i = 0; i < clef->len; i++) {
        hash = 33 * hash ^ ptrclef[i];
    }
    return hash % 999983;  // modulo pour éviter de dépasser
}

// FNV_hash
int fnv_hash(const kv_datum* clef) {
    unsigned char* ptrclef = clef->ptr;
    unsigned h = 2166136261;
    unsigned int i;

    for (i = 0; i < clef->len; i++) {
        h = (h * 16777619) ^ ptrclef[i];
    }

    return h % 999983;  // modulo pour éviter de dépasser
}

//****************************************************************************//

// Modification

// Fonction qui change le int stocké à partir de début dans buf
unsigned char* modif(unsigned char* buf, int i, int debut) {
    buf[debut] = i & 0xFF;
    buf[debut + 1] = (i >> 8) & 0xFF;
    buf[debut + 2] = (i >> 16) & 0xFF;
    buf[debut + 3] = (i >> 24) & 0xFF;
    return buf;
}

//****************************************************************************//

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

// fonction qui concatene 2 chaines de caractères
char* concat(const char* S1, const char* S2) {
    char* result = malloc(strlen(S1) + strlen(S2) + 1);
    strncpy(result, S1, strlen(S1));
    strncpy(result + strlen(S1), S2, strlen(S2));
    result[strlen(S1) + strlen(S2)] = '\0';
    return result;
}

// prend comme argument le numéro de bloc (commençant par 1)
// retourne 1 si réussi -1 sinon
int remplit_bloc(int debut, KV* kv) {
    int test;
    char buffbloc[4096];
    for (unsigned int i = debut; i < kv->longueur_buf_bloc; i++) {
        if (lseek(kv->descr_blk, LG_EN_TETE_BLK + (i)*4096, SEEK_SET) == -1)
            return -1;
        if ((test = read(kv->descr_blk, &buffbloc, 4096)) == -1)
            return -1;

        if (test == 0)  // il n'y a aucun bloc
        {
            kv->tabbloc[i] = 0;  // indique que le bloc est vide
        } else {
            // regarde si le nombre d'emplacements est vide
            if (*(int*)(buffbloc + 5) == 0)
                kv->tabbloc[i] = 0;  // indique que le bloc est vide
            else
                kv->tabbloc[i] = 1;  // indique que le bloc est occupé
        }
    }
    return 1;
}

// recherche le numéro de bloc contenu à la position hache du fichier .h
// numéro retourné : numbloc + 1 (vu que les blocs commencent à 1)
// en tête bloc : bloc suivant(char)+nr bloc suivant(int) + nb emplacements (int)
// retourne 0 si aucun bloc alloué, -1 s'il y a une erreur, i>0 si bloc trouvé
int RechercheBlocH(KV* kv, int hache) {
    int numbloc;
    int test;
    int fils;
    unsigned char* bloctmp;

    if (lseek(kv->descr_h, LG_EN_TETE_H + hache * 4, SEEK_SET) == -1)
        return -1;

    // regarde si un bloc est alloué
    if ((test = read(kv->descr_h, &numbloc, 4)) == -1)
        return -1;

    // soit rien n'était écrit à cet emplacement, soit 0 y était écrit
    // donc il n'y a pas de bloc alloué
    if (test == 0 || numbloc == 0) {
        return 0;
    } else {
        // dans ce cas il faut encore regarder si le bloc est vide,
        // puis si son fils est vide, etc... Jusqu'à trouver un emplacement vide
        kv->bloc = malloc(4096);
        while (1) {
            if (lseek(kv->descr_blk, (numbloc - 1) * 4096 + LG_EN_TETE_BLK, SEEK_SET) == -1)
                return -1;
            if (read(kv->descr_blk, kv->bloc, 4096) == -1)  // lit le bloc
                return -1;
            if (*(int*)(kv->bloc + 5) + 2 <
                (4096 - LG_EN_TETE_BLOC) / 4) {  // si le nombre actuel d'emplacements +1 (celui qu'on ajoute)
                                                 // est inférieur au nb max d'emplacements d'un bloc
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
                bloctmp = modif(bloctmp, fils, 1);  // écrit le fils dans le bloc
                if (lseek(kv->descr_blk, LG_EN_TETE_BLK + 4096 * (numbloc - 1), SEEK_SET) == -1)
                    return -1;
                write_controle(kv->descr_blk, bloctmp, LG_EN_TETE_BLOC + (*(int*)(bloctmp + 5)) * 4);  // écrit le bloc
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
        // se place après l'en tête de blk + i bloc
        if (lseek(kv->descr_blk, LG_EN_TETE_BLK + (numbloc - 1) * 4096, SEEK_SET) == -1)
            return 0;
        if ((test = read(kv->descr_blk, kv->bloc, 4096)) == -1)
            return 0;
    }
    unsigned int nb_emplcts_occupes = *(int*)(kv->bloc + 5);
    for (unsigned int i = 0; i < nb_emplcts_occupes; i++) {  // passe tous les emplacements occupés dans le bloc
        test = compareClefkv(kv, key, *(int*)(kv->bloc + LG_EN_TETE_BLOC + 4 * i));
        if (test == -1) {
            errno = EINVAL;
            return 0;
        }
        if (test == 1)
            return (*(int*)(kv->bloc + LG_EN_TETE_BLOC + 4 * i) + 1);
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
    len_t longueur = 0;  // longueur de la clef
    len_t nb_total_lus = 0;
    int nb_octets_lus = 0;
    unsigned char buf[2048];
    unsigned int i;
    unsigned char* ptrkey = key->ptr;  // cast le pointeur de key
    unsigned int maxoctetslus = 0;     // lis au max 512 octets
    // se déplace d'abord au bonne offset dans .kv
    if (lseek(kv->descr_kv, offset + LG_EN_TETE_KV, SEEK_SET) == -1)
        return -1;
    // lecture de la longueur de la clef -> 4 octets
    if (read_controle(kv->descr_kv, &longueur, 4) == -1)
        return -1;
    if (longueur != key->len)  // si les longueurs sont différentes
        return 0;
    while (nb_total_lus < longueur)  // évite de faire un malloc énorme
    {                                // lis au max 2048 octets d'un coup
        maxoctetslus = (longueur - nb_total_lus) % 2049;
        nb_octets_lus = read_controle(kv->descr_kv, buf, maxoctetslus);
        if (nb_octets_lus == -1)
            return -1;
        //        if (memcmp(ptrkey + nb_total_lus, buf, maxoctetslus) == 0)
        //          return 1;

        for (i = 0; i < maxoctetslus; i++) {
            if (ptrkey[i + nb_total_lus] != buf[i])  // si 2 octets sont différents
                break;                               // sort du for
        }
        nb_total_lus += (len_t)nb_octets_lus;
        if (i < maxoctetslus)
            break;  // si i!=longueur-> 2 octets sont != car for n'a pas terminé
    }
    if (i == longueur)
        return 1;

    return 0;
}

// retourne la taille de la clef ou 0 si erreur
len_t RechercheTailleClef(KV* kv, len_t offset) {
    len_t longueur;
    // se positionne d'abord à l'offset nécessaire dans le fichier
    if (lseek(kv->descr_kv, offset + LG_EN_TETE_KV, SEEK_SET) == -1)
        return 0;
    if (read_controle(kv->descr_kv, &longueur, 4) == -1)  // cherche longueur de la clef
        return 0;
    return (longueur);  // retourne la longueur de la valeur
}

// Remplit la valeur
// retourne 1 si réussi, -1 sinon
int RemplissageVal(KV* kv, len_t offset, kv_datum* val, const kv_datum* key) {
    len_t longueur = key->len;
    // Vérifie si le pointeur de val est NULL ou si la longueur est déjà définie
    if (val == NULL) {
        val = malloc(sizeof(struct kv_datum));
        val->ptr = NULL;
    }
    if (val->ptr == NULL)  // dans ce cas cherche la longueur à allouer
    {
        // se positionne d'abord à l'offset nécessaire dans le fichier
        if (lseek(kv->descr_kv, offset + LG_EN_TETE_KV + longueur + 4, SEEK_SET) == -1)
            return -1;
        // enregistre la longueur de la valeur
        if (read_controle(kv->descr_kv, &longueur, 4) == -1)
            return -1;
        val->len = longueur;
        val->ptr = malloc(val->len);
    } else  // val->len est le max d'octets que l'on peut lire
    {
        // compare val->len et la longueur de la valeur stockée
        // se positionne d'abord à l'offset nécessaire dans le fichier
        if (lseek(kv->descr_kv, offset + LG_EN_TETE_KV + longueur + 4, SEEK_CUR) == -1)
            return -1;
        // enregistre la longueur de la valeur
        if (read_controle(kv->descr_kv, &longueur, 4) == -1)
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

// Remplit la clef
// retourne 1 si réussi, -1 sinon
int RemplissageClef(KV* kv, len_t offset, kv_datum* clef) {
    len_t longueur_a_allouer = 0;
    len_t longueur = 0;
    // Vérification si val a déjà été alloué et si la longueur est déjà définie
    if (clef == NULL) {
        clef = malloc(sizeof(struct kv_datum));
        clef->ptr = NULL;
    }
    if (clef->ptr == NULL)  // dans ce cas on cherche la longueur à allouer
    {
        longueur_a_allouer = RechercheTailleClef(kv, offset);
        if (longueur_a_allouer == 0)  // erreur
            return -1;
        clef->ptr = malloc(longueur_a_allouer);
        clef->len = longueur_a_allouer;
    } else  // clef->len est le max d'octets que l'on peut lire
    {
        // recherche d'abord le max entre clef->len et la longueur de la valeur
        // stockéese positionne d'abord à l'offset nécessaire dans le fichier
        if (lseek(kv->descr_kv, offset + LG_EN_TETE_KV, SEEK_SET) == -1)
            return -1;
        if (read_controle(kv->descr_kv, &longueur, 4) == -1)  // longueur de la clef
            return -1;
        // affecte à clef->len longueur si longueur<val->len
        clef->len = (longueur < clef->len) ? longueur : clef->len;
    }
    if (read_controle(kv->descr_kv, clef->ptr, clef->len) == -1)
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
int liaisonHBlk(KV* kv, int hache, int numbloc) {
    // écrit dans .h numbloc
    // se place
    if (lseek(kv->descr_h, LG_EN_TETE_H + hache * 4, SEEK_SET) == -1)
        return -1;
    if (write_controle(kv->descr_h, &numbloc, 4) == -1)
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
    int nb_emplacementstmp = *(int*)kv->dkv;
    if (emplacement_dkv != 0 && access_lg_dkv(emplacement_dkv - 1, kv) > 0)
        voisinp = emplacement_dkv - 1;
    if (emplacement_dkv + 1 != *(int*)kv->dkv && access_lg_dkv(emplacement_dkv + 1, kv) > 0)
        voisins = emplacement_dkv + 1;
    fusionVoisinsVidesSP(voisins, emplacement_dkv, voisinp, kv);
    // si l'emplacement est le dernier le supprime (diminue le nb d'emplacments
    if (emplacement_dkv == *(int*)kv->dkv || emplacement_dkv + 1 == nb_emplacementstmp) {
        kv->dkv = modif(kv->dkv, (*(int*)kv->dkv) - 1, 0);
        kv->remplissement_dkv -= 8;
    }
}

void fusionVoisinsVidesSP(int voisins, int emplacement_dkv, int voisinp, KV* kv) {
    if (voisinp != -1)  // si le voisin précédent est vide
    {
        // ajoute la taille de l'emplacement voisinp à emplacement_dkv
        kv->dkv =
            modif(kv->dkv, access_lg_dkv(emplacement_dkv, kv) + access_lg_dkv(voisinp, kv), 4 + 8 * emplacement_dkv);
        // modifie l'offset et le met à celui de voisinp
        kv->dkv = modif(kv->dkv, access_offset_dkv(voisinp, kv), 4 + 8 * emplacement_dkv + 4);
        // libère voisinp
        decaledkv_arriere(kv, voisinp);
        // diminue le nombre d'emplacements
        kv->dkv = modif(kv->dkv, *(int*)kv->dkv - 1, 0);
        // décrémente le remplissement de dkv
        kv->remplissement_dkv -= 8;
        // supprime le dernier emplacement -> ftruncate dans close
    }
    if (voisins != -1)  // si le voisin suivant est vide
    {
        if (voisinp != -1) {
            voisins--;
            emplacement_dkv--;
        }
        // ajoute la taille de voisins à emplacement_dkv
        kv->dkv =
            modif(kv->dkv, access_lg_dkv(emplacement_dkv, kv) + access_lg_dkv(voisins, kv), 4 + 8 * emplacement_dkv);
        // libère voisins
        decaledkv_arriere(kv, voisins);
        // diminue le nombre d'emplacements
        kv->dkv = modif(kv->dkv, *(int*)kv->dkv - 1, 0);
        // décrémente le remplissement de dkv
        kv->remplissement_dkv -= 8;
        // supprime le dernier emplacement -> ftruncate dans close
    }
    return;
}

// décalle tout dkv vers une position avant
void decaledkv_arriere(KV* kv, int emplacement_a_sup) {
    int nb_emplacements_dkv = *(int*)kv->dkv;
    int tmp_lg;
    int tmp_offset;
    for (int i = emplacement_a_sup; i < nb_emplacements_dkv - 1; i++) {
        tmp_lg = access_lg_dkv(i + 1, kv);
        tmp_offset = access_offset_dkv(i + 1, kv);
        kv->dkv = modif(kv->dkv, tmp_lg, 4 + 8 * i);
        kv->dkv = modif(kv->dkv, tmp_offset, 4 + 8 * i + 4);
    }
    return;
}

// libère un emplacement identifié par son offset dans .dkv
void libereEmplacementdkv(len_t offset, KV* kv) {
    unsigned int nb_emplacements = *(int*)kv->dkv;
    len_t offsetdkv;
    for (unsigned int i = 0; i < nb_emplacements; i++) {
        // saute  nb_emplacements + i*(1+lg) + i*offset
        offsetdkv = access_offset_dkv(i, kv);
        if (offsetdkv == offset) {
            // on stock |lg| dans la longueur, or lg < 0 donc on stock -lg
            kv->dkv = modif(kv->dkv, access_lg_dkv(i, kv) * (-1), 4 + 8 * i);
            insertionFusionEspace(kv, i);
            // faut voir si l'emplacement se situe tout à droite (si c'est un
            // emplacement vide à la fin de la base et s'il faut le supprimer)
            if (i == nb_emplacements) {
                // enlève un emplacement pour le libérer
                kv->dkv = modif(kv->dkv, nb_emplacements - 1, 0);
                kv->remplissement_dkv -= 8;
            }
            return;
        }
    }
    return;
}

// libère un emplacement dans blk
// en-tête bloc:  bloc suivant(char)+nr bloc suivant(int) + nb emplacements (int)
// bloc pere vaut 0 s'il y en a pas, le num du bloc père sinon
// retourne 1 si réussi, -1 sinon
int libereEmplacementblk(int numbloc, len_t offset, KV* kv, int hache, int bloc_p) {
    unsigned int nb_emplacements, i;
    len_t offset_d_emplct;
    // enregistre le nombre d'emplacements
    nb_emplacements = *(unsigned int*)(kv->bloc + 5);
    // enregistre l'offset du dernier emplacement
    offset_d_emplct = *(len_t*)(kv->bloc + 4 * (nb_emplacements - 1) + LG_EN_TETE_BLOC);
    // recherche d'abord l'emplacement à supprimer
    for (i = 0; i < nb_emplacements; i++) {
        // Si on a trouvé le bon emplacement
        if (offset == *(len_t*)(kv->bloc + LG_EN_TETE_BLOC +
                                i * 4)) {  // dans ce cas on met l'offset du dernier emplacement à la place i
            // remplace l'offset contenu dans i par l'offset du dernier emplct
            kv->bloc = modif(kv->bloc, offset_d_emplct, LG_EN_TETE_BLOC + 4 * i);
            // décrémente le nb d'emplacements
            kv->bloc = modif(kv->bloc, nb_emplacements - 1, 5);
            break;  // emplacement trouvé donc sort de for
        }
    }
    // indique que l'emplacement a pas été trouvé
    if (i == nb_emplacements || nb_emplacements == 0) {
        free(kv->bloc);
        // rééexcute la fonction avec le bloc prochain(c'est sûr qu'il y en a un)
        //-> pour savoir s'il y en a un on le * -1 (car il faudra le lire)
        return libereEmplacementblk((-1) * (*(int*)(kv->bloc + 1)), offset, kv, hache, numbloc);
    } else {
        // dans ce cas on libère le bloc dans .h
        if (nb_emplacements - 1 == 0 && bloc_p == 0 && kv->bloc[0] == 0) {
            if (supprimeblocdeh(kv, hache) == -1)
                return -1;
            kv->nb_blocs--;
            kv->tabbloc[numbloc - 1] = 0;  // libère le bloc
        }

        // se place au bon endroit
        if (lseek(kv->descr_blk, LG_EN_TETE_BLK + (numbloc - 1) * 4096, SEEK_SET) == -1)
            return -1;
        // écrit le bloc
        if (write_controle(kv->descr_blk, kv->bloc, LG_EN_TETE_BLOC + (nb_emplacements - 1) * 4) == -1) {
            free(kv->bloc);
            return -1;
        }
        free(kv->bloc);
    }
    return 1;
}

// Supprime la référence du bloc dans .h
// retourne -1 en cas d'erreur sinon retourne 1
int supprimeblocdeh(KV* kv, int hache) {
    int i = 0;
    // libère l'emplacement dans la table de hachage
    // se place au bon endroit
    if (lseek(kv->descr_h, LG_EN_TETE_H + hache * 4, SEEK_SET) == -1)
        return -1;
    if (write_controle(kv->descr_h, &i, 4) == -1)  // écrit 0 au numéro de bloc
        return -1;
    return 1;
}

// création d'un nouvel emplacement à la fin de dkv
// l'endroit de cet emplacement sera égal à nb_emplacements-1
// en-tête dkv :MagicN+ nb_d'emplacements
void NouvEmplacementDkv(KV* kv, const kv_datum* key, const kv_datum* val) {
    int nb_emplacements = *(int*)kv->dkv;  // nombre d'emplacements
    int taille_emplacement_necessaire = 4 + key->len + 4 + val->len;
    len_t offset_emplacement;
    if (nb_emplacements == 0)  // au début, lors de la création
    {
        kv->dkv = modif(kv->dkv, 1, 0);  // met le nb d'emplacements à 1
        kv->dkv = modif(kv->dkv, taille_emplacement_necessaire, 4);
        offset_emplacement = 0;
        kv->dkv = modif(kv->dkv, offset_emplacement, 8);
        kv->remplissement_dkv += 8;  // incrémente le remplissement de dkv
        return;
    }
    // ajoute un emplacement à la fin de dkv
    kv->dkv = modif(kv->dkv, nb_emplacements + 1, 0);  // incrémente le nombre d'emplcts
    kv->remplissement_dkv += 8;                        // incrémente le remplissement de dkv
    if (kv->longueur_dkv < kv->remplissement_dkv)      // le buffer n'est pas assez grand
    {
        kv = dkv_aggrandissement(kv);  // réalloue de la place : aggrandit le buffer
    }
    // initialise l'offset max à celui de la première valeur
    len_t offset_max = access_offset_dkv(nb_emplacements - 1, kv);
    // longueur de l'emplacement avec l'offset max
    int longueurmax = abs(access_lg_dkv(nb_emplacements - 1, kv));
    kv->dkv = modif(kv->dkv, offset_max + longueurmax, 4 + nb_emplacements * 8 + 4);
    kv->dkv = modif(kv->dkv, taille_emplacement_necessaire, 4 + nb_emplacements * 8);
    return;
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
    int emplacement_vide = *(int*)(kv->bloc + 5);
    // indique qu'on a changé de bloc (emplacement_vide contient le num du bloc)
    if (emplacement_vide == -1)
        return -1;

    // se place au bon endroit dans .blk
    kv->bloc = modif(kv->bloc, emplacement_kv, LG_EN_TETE_BLOC + emplacement_vide * 4);

    // incrémente le nb d'emplacements dans le bloc
    emplacement_vide++;  // incrémente nb emplacements
    kv->bloc = modif(kv->bloc, emplacement_vide, 5);
    // Pour finir écrit le bloc
    if (lseek(kv->descr_blk, LG_EN_TETE_BLK + 4096 * bloc, SEEK_SET) == -1)
        return -1;
    if (write_controle(kv->descr_blk, kv->bloc, LG_EN_TETE_BLOC + 4 * (*(int*)(kv->bloc + 5))) == -1)
        return -1;
    free(kv->bloc);

    return 1;
}

// met l'emplacement en tant que place occupée dans dkv
KV* SetOccupeDkv(KV* kv, int emplacement_dkv) {
    kv->dkv = modif(kv->dkv, access_lg_dkv(emplacement_dkv, kv) * (-1),
                    LG_EN_TETE_DKV + emplacement_dkv * 8 - 1);  // on a pas prit le magic number
    return kv;
}

// voit s'il y a encore de la place pour une autre clef + valeur
// puis insérer cet nouvel espace dans dkv
// emplacement_dkv : numéro de l'emplacement dans dkv (i)
void insertionNewEspace(KV* kv, int emp_dkv, const kv_datum* key, const kv_datum* val) {
    int nbemplacements = *(int*)kv->dkv;
    len_t taille_couple = 4 + key->len + 4 + val->len;
    int octets_restants = (-1) * access_lg_dkv(emp_dkv, kv) - (taille_couple);
    // regarde si place pour une autre clef
    if (octets_restants - 10 > 0)  // s'il reste plus de 10 octets (2int + 2 octets)
    {
        // diminue la longueur de emplacement_kv à la taille du couple
        kv->dkv = modif(kv->dkv, taille_couple * (-1), 4 + 8 * emp_dkv);
    } else  // sinon retourne kv
        return;

    if (emp_dkv == nbemplacements - 1)
        return;
    // si n'a pas pu fusionner avec le voisin précédent, fait un nouvel emplct
    creationNewVoisin(kv, octets_restants, emp_dkv + 1, taille_couple);
    return;
}

// créé un nouvel emplacement
void creationNewVoisin(KV* kv, int octets_restants, int empl_dkv, len_t t_couple) {
    // décalle d'abord tout dkv vers une position à l'avant
    decaledkv_avant(kv, empl_dkv);
    // créé le nouvel emplacement
    // met la longueur à octets_restants
    kv->dkv = modif(kv->dkv, octets_restants, 4 + (empl_dkv)*8);
    kv->dkv = modif(kv->dkv, access_offset_dkv(empl_dkv - 1, kv) + t_couple, 4 + (empl_dkv)*8 + 4);
    return;
}

// décalle tout dkv vers une position avant
void decaledkv_avant(KV* kv, int emplacement_a_decal) {
    int i = emplacement_a_decal;
    int nb_emplacements_dkv = *(int*)kv->dkv;
    int tmp_lg;
    int tmp_offset;
    int tmp_lg_suiv;
    int tmp_offset_suiv;
    int nb_emplacements = *(int*)kv->dkv + 1;
    // incrémente le nombre d'emplacements
    kv->dkv = modif(kv->dkv, nb_emplacements, 0);
    // incrémente le remplissement de dkv
    kv->remplissement_dkv += 8;
    // regarde si toujours assez de place
    if (kv->longueur_dkv < kv->remplissement_dkv)  // le buffer n'est plus assez grand
        kv = dkv_aggrandissement(kv);

    tmp_lg_suiv = access_lg_dkv(i + 1, kv);
    tmp_offset_suiv = access_offset_dkv(i + 1, kv);
    tmp_lg = access_lg_dkv(i, kv);
    tmp_offset = access_offset_dkv(i, kv);
    kv->dkv = modif(kv->dkv, tmp_lg, 4 + 8 * (i + 1));
    kv->dkv = modif(kv->dkv, tmp_offset, 4 + 8 * (i + 1) + 4);
    for (i = emplacement_a_decal + 1; i < nb_emplacements_dkv; i++) {
        tmp_lg = tmp_lg_suiv;
        tmp_offset = tmp_offset_suiv;
        tmp_lg_suiv = access_lg_dkv(i + 1, kv);
        tmp_offset_suiv = access_offset_dkv(i + 1, kv);
        kv->dkv = modif(kv->dkv, tmp_lg, 4 + 8 * (i + 1));
        kv->dkv = modif(kv->dkv, tmp_offset, 4 + 8 * (i + 1) + 4);
    }
    return;
}

// tronque kv
int truncate_kv(KV* kv) {
    if (strcmp(kv->mode, "r") == 0)  // pas besoin de tronquer vu que rien n'a changé
        return 1;
    len_t offset_max = 0;
    len_t longueur_max = 0;
    unsigned int nb_emplacements_dkv = *(int*)kv->dkv;
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
