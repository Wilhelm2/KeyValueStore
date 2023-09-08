// WILHELM Daniel YAMAN Kendal
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>
#include "kv.h"

void affiche_base(KV* kv);
int del(int i, int j, KV* kv, char* chaine);
void remplit_base(int n, KV* kv, char* chaine);

// remplit la base avec des clefs allant de 0 à i-1 et des valeurs allant de 1 à i
void remplit_base(int n, KV* kv, char* chaine) {
    int i;
    kv_datum key, val;
    key.ptr = chaine;
    val.ptr = chaine;
    for (i = 0; i < n; i++) {
        key.len = i + 1;
        val.len = i + 2;
        kv_put(kv, &key, &val);
    }
    return;
}

// fonction qui supprime tous les clefs de valeur entre i et j
// la base est déjà ouverte
// retourne -1 si un des del n'a pas fonctionné sinon retourne
int del(int i, int j, KV* kv, char* chaine) {
    int test = 0;
    int k;
    kv_datum key;
    key.ptr = chaine;
    for (k = i; k < j; k++) {
        key.len = k + 1;
        if (kv_del(kv, &key) == -1)
            test = -1;
    }

    return test;
}

// affiche toute la base
// la base est ouverte
void affiche_base(KV* kv) {
    kv_datum key;
    kv_datum val;
    printf("------------------------------\n     CONTENU DE LA BASE :     \n------------------------------\n");
    kv_start(kv);
    key.ptr = NULL;
    val.ptr = NULL;
    while (kv_next(kv, &key, &val) == 1) {
        printf("CLEF : longueur %u, valeur \n", key.len);
        write(1, key.ptr, key.len);
        printf("\nVal : longueur %u valeur \n", val.len);
        write(1, val.ptr, val.len);
        printf("\n");
        free(key.ptr);
        free(val.ptr);
        key.ptr = NULL;
        val.ptr = NULL;
    }
    printf("------------------------------\n       FIN DE LA BASE        \n------------------------------\n");
}

// fonction qui stock des chaines de 0 dans la base.
int main(int argc, char* argv[]) {
    int taille;  // par défaut met 1000 valeurs
    alloc_t a;   // par défaut met alloc à first fit
    int i;
    int hidx = 0;
    if (argc < 5) {
        printf("usage %s : <nom de base> <taille> <methode d'allocation> <mode> <hidx>\n", argv[0]);
        exit(1);
    }
    taille = atoi(argv[2]);
    a = atoi(argv[3]);
    if (argc == 6)
        hidx = atoi(argv[5]);

    // création de la châine qui sert pour remplir la base
    char* chaine = malloc(taille + 1);
    for (i = 0; i <= taille; i++)
        chaine[i] = '0';

    KV* kv = kv_open(argv[1], argv[4], hidx, a);
    if (kv == NULL) {
        printf("erreur\n");
        exit(1);
    }

    //	printf("remplit la base\n");
    remplit_base(taille - 1, kv, chaine);

    //	printf("supprime les %d premiers éléments\n",taille/2);
    del(0, taille / 2, kv, chaine);
    //	affiche_base(kv);
    if (kv_close(kv) == -1)
        printf("erreur\n");

    free(chaine);

    return 0;
}
