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

// Main qui couvre le plus de code possible pour gcov

#define TAILLE 10000

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

// créé un tableau de mots de longueur max lenght_max
kv_datum* mot(int lenght_max) {
    kv_datum* kv = malloc(sizeof(kv_datum));
    char* tab;
    char lettre;
    char lettrestring[2];
    int lenght = rand() % (lenght_max + 1);
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
    //	printf("------------------------------\n     CONTENU DE LA BASE :     \n------------------------------\n");
    kv_start(kv);
    key.ptr = NULL;
    val.ptr = NULL;
    while (kv_next(kv, &key, &val) == 1) {
        //		printf("CLEF :longueur %d, valeur %s\n", key.len,verifc);
        //		printf("VAL : longueur %d, valeur %s\n", val.len,verifv);
        //		printf("--\n");
        free(key.ptr);
        free(val.ptr);
        key.ptr = NULL;
        val.ptr = NULL;
    }
    //	printf("------------------------------\n       FIN DE LA BASE        \n------------------------------\n");
}

int main() {
    int i;
    kv_datum** tableau = creation_tableau(TAILLE);
    kv_datum** tableau_mot = creation_tableau_mot(TAILLE, 50);
    kv_datum* key;
    kv_datum* val;
    KV* kv;
    kv = kv_open("database", "r+", 0, FIRST_FIT);
    if (kv == NULL) {
        printf("erreur open\n");
        exit(1);
    }
    // remplit le tableau avec la méthode first fit
    remplit_base(5000, kv, tableau);
    if (kv_close(kv) == -1)
        printf("erreur close\n");

    kv = kv_open("database", "r+", 0, BEST_FIT);
    if (kv == NULL) {
        printf("erreur open\n");
        exit(1);
    }
    // remplit le tableau avec la méthode best fit
    remplit_base(5000, kv, tableau_mot);

    if (kv_close(kv) == -1)
        printf("erreur close\n");

    kv = kv_open("database", "r+", 0, WORST_FIT);
    if (kv == NULL) {
        printf("erreur open\n");
        exit(1);
    }
    // remplit le tableau avec la méthode worst fit
    remplit_base(100, kv, tableau);

    // supprime des valeurs
    for (i = 0; i < 750; i++) {
        kv_del(kv, tableau_mot[i * 2]);
        kv_del(kv, tableau_mot[i * 3]);
    }
    if (kv_close(kv) == -1)
        printf("erreur close\n");

    // lit une valeur
    kv = kv_open("database", "r", 0, FIRST_FIT);
    if (kv == NULL) {
        printf("erreur open\n");
        exit(1);
    }
    key = tableau[1];
    val = malloc(sizeof(kv_datum));
    val->ptr = NULL;
    kv_get(kv, key, val);
    val->ptr = NULL;
    key = tableau[11];
    kv_get(kv, key, val);
    // get d'une clef avec val limité -> val->ptr n'est pas réinistialisé
    kv_get(kv, key, val);
    free(val->ptr);

    // get d'une clef inexistante
    key = malloc(sizeof(kv_datum));
    char* tmp = "nimportequoi";
    key->ptr = tmp;
    key->len = 13;
    val->ptr = NULL;
    kv_get(kv, key, val);

    // affiche la base (modifiée pour ne rien afficher) -> exécute kv_start et kv_next
    affiche_base(kv);
    free(val);
    free(key);
    if (kv_close(kv) == -1)
        printf("erreur close\n");

    // créé une nouvelle base avec comme index de hachage 1 qui renvoie vers une
    // fonction qui retourne toujours 0 afin de tester les mécanismes bloc
    // fils/père
    kv = kv_open("database2", "w", 1, FIRST_FIT);
    if (kv == NULL) {
        printf("erreur open\n");
        exit(1);
    }
    // remplit la base avec assez de valeur pour changer de bloc
    remplit_base(4000, kv, tableau);
    // get d'une clef inexistante
    kv_get(kv, tableau[1500], tableau[4]);

    // supprime 1500 emplacements
    del(0, 1500, kv, tableau);
    if (kv_close(kv) == -1)
        printf("erreur close\n");

    kv = kv_open("database3", "r+", 2, FIRST_FIT);
    if (kv == NULL) {
        printf("erreur open\n");
        exit(1);
    }
    // remplit le tableau avec la méthode first fit
    remplit_base(10, kv, tableau);
    del(0, 10, kv, tableau);
    if (kv_close(kv) == -1)
        printf("erreur close\n");
    kv = kv_open("database4", "r+", 3, FIRST_FIT);
    if (kv == NULL) {
        printf("erreur open\n");
        exit(1);
    }
    // remplit le tableau avec la méthode first fit
    remplit_base(10, kv, tableau);
    del(0, 10, kv, tableau);
    if (kv_close(kv) == -1)
        printf("erreur close\n");

    for (i = 0; i <= TAILLE; i++) {
        free(tableau[i]->ptr);
        free(tableau[i]);
        free(tableau_mot[i]->ptr);
        free(tableau_mot[i]);
    }
    free(tableau);
    free(tableau_mot);
    return 0;
}
