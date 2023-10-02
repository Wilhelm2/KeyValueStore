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
int del(int i, int j, KV* kv, kv_datum** tableau);
void remplit_base(int n, KV* kv, kv_datum** tableau);
kv_datum** creation_tableau(int n);
kv_datum* transformation(int i);
int puissance_dix(int i);

int puissance_dix(int i) {
    int s = 1, j;
    for (j = 0; j < i; j++)
        s *= 10;
    return s;
}

// transforme i en tab de char *
kv_datum* transformation(int i) {
    int j = 0, k;
    int n = i;
    kv_datum* kv = malloc(sizeof(kv_datum));
    char* tab;
    while (n > 0) {
        n = n / 10;
        j++;
    }
    if (i == 0) {
        tab = malloc(2);
        tab[0] = '0';
        tab[1] = '\0';
        kv->ptr = tab;
        kv->len = 2;
        return kv;
    }
    tab = malloc(j + 1);
    for (k = 0; k < j; k++) {
        tab[k] = i / puissance_dix(j - k - 1) % 10 + '0';
    }
    tab[j] = '\0';
    kv->ptr = tab;
    if (kv->ptr == NULL) {
        exit(5);
    }
    kv->len = j + 1;
    return kv;
}

// créé un tableau de kv_datum de taille i+1, de nombre sous forme string de 0 à i
kv_datum** creation_tableau(int n) {
    int i;
    kv_datum* kv;
    kv_datum** tableau = malloc(sizeof(kv_datum*) * (n + 1));
    for (i = 0; i <= n; i++) {
        kv = transformation(i);
        tableau[i] = kv;
    }
    return tableau;
}

// remplit la base avec des clefs allant de 0 à i-1 et des valeurs allant de 1 à i
void remplit_base(int n, KV* kv, kv_datum** tableau) {
    int i;
    for (i = 0; i < n; i++) {
        kv_put(kv, tableau[i], tableau[i + 1]);
    }
    return;
}

// fonction qui supprime tous les clefs de valeur entre i et j
// la base est déjà ouverte
// retourne -1 si un des del n'a pas fonctionné sinon retourne
int del(int i, int j, KV* kv, kv_datum** tableau) {
    int test = 0;
    int k;
    for (k = i; k < j; k++) {
        if (kv_del(kv, tableau[k]) == -1)
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

int main(int argc, char* argv[]) {
    int taille;  // par défaut met 1000 valeurs
    alloc_t a;   // par défaut met alloc à first fit
    int i;
    int hidx = 0;
    if (argc < 5) {
        printf("usage %s : <nom de base> <taille> <methode d'allocation> <mode>\n", argv[0]);
        exit(1);
    }
    taille = atoi(argv[2]);
    a = atoi(argv[3]);
    if (argc == 6)
        hidx = atoi(argv[5]);

    KV* kv = kv_open(argv[1], argv[4], hidx, a);
    if (kv == NULL) {
        printf("erreur\n");
        exit(1);
    }
    kv_datum** tableau = creation_tableau(taille);
    //	printf("remplit la base\n");
    remplit_base(taille - 1, kv, tableau);

    //	printf("supprime les %d premiers éléments\n",taille/2);
    del(0, taille / 2, kv, tableau);
    //	affiche_base(kv);
    if (kv_close(kv) == -1)
        printf("erreur\n");

    for (i = 0; i <= taille; i++) {
        free(tableau[i]->ptr);
        free(tableau[i]);
    }
    free(tableau);

    return 0;
}
