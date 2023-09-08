// WILHELM Daniel YAMAN Kendal
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/times.h>
#include <time.h>
#include <unistd.h>
#include "kv.h"

void affiche_base(KV* kv);
int del(int i, int j, KV* kv, kv_datum** tableau);
void remplit_base(int n, KV* kv, kv_datum** tableau);
kv_datum** creation_tableau(int n);
kv_datum** creation_tableau_mot(int n, int taillemot);
kv_datum* mot(int lenght_max);

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

kv_datum* mot(int lenght_max) {
    kv_datum* kv = malloc(sizeof(kv_datum));
    char* tab;
    char lettre;
    char lettrestring[2];
    char lenght = rand() % (lenght_max + 1);
    lettrestring[1] = '\0';
    int i;
    tab = malloc(lenght + 1);
    strcpy(tab, "");
    for (i = 0; i < lenght; ++i) {
        lettre = 'a' + (rand() % 27);
        lettrestring[0] = lettre;
        strcat(tab, lettrestring);
    }
    kv->ptr = tab;
    kv->len = lenght + 1;

    return kv;
}

kv_datum** creation_tableau_mot(int n, int taillemot) {
    int i;
    kv_datum* kv;
    kv_datum** tableau = malloc(sizeof(kv_datum*) * (n + 1));
    for (i = 0; i <= n; i++) {
        kv = mot(taillemot);
        tableau[i] = kv;
    }
    return tableau;
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
    int taillemot;
    if (argc < 6) {
        printf("usage %s : <nom de base> <taille> <taille_max_mot> <methode d'allocation> <mode> <hidx>\n", argv[0]);
        exit(1);
    }
    taille = atoi(argv[2]);
    taillemot = atoi(argv[3]);
    a = atoi(argv[4]);
    if (argc == 6)
        hidx = atoi(argv[6]);

    // création de la châine qui sert pour remplir la base
    kv_datum** tableau = malloc(sizeof(kv_datum) * (taille + 1));
    tableau = creation_tableau_mot(taille, taillemot);
    KV* kv = kv_open(argv[1], argv[5], hidx, a);
    if (kv == NULL) {
        printf("erreur\n");
        exit(1);
    }

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
