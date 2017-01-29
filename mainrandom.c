// WILHELM Daniel YAMAN Kendal

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include "kv.h"
#include <errno.h>
#include <time.h>

kv_datum * table(int lenght);
kv_datum * transformation(int i);
void affiche_base(KV * kv);
void test_random(KV *kv, int taille,int taillefor,kv_datum** tableau);
int del (int i, int j, KV*kv,kv_datum **tableau);
void remplit_base(int n,KV *kv,kv_datum **tableau);
kv_datum ** creation_tableau_random(int n, int MAX);
kv_datum * mot(int lenght_max);
kv_datum ** creation_tableau_random2(int n, int MAX);



typedef struct KV
{
	int descr_h;
	int descr_blk;
	int descr_kv;
	int descr_dkv;
    alloc_t methode_alloc;
    unsigned char * dkv;
    len_t remplissement_dkv;
    len_t longueur_dkv;
    const char * mode;
    int couple_nr_kv_next;
    int (*f_hachage)(const kv_datum* kdatum);
    unsigned char *bloc;
    int key_long;
    int * tabbloc;
    int longueur_buf_bloc;
    int nb_blocs;
}KV ;




int puissance_dix(int i)
{
	int s=1,j;
	for(j=0;j<i;j++) s*=10;
	return s;
}

//transforme i en tab de char *
kv_datum * transformation(int i)
{
	int j=0,k;
	int n=i;
	kv_datum *kv=malloc(sizeof( kv_datum));
	char * tab;
	while(n>0)
	{
		n=n/10;
		j++;
	}
	if(i==0) 
	{
		tab=malloc(2);
		tab[0]='0';
		tab[1]='\0';
		kv->ptr=tab;
		kv->len=2;
		return kv;
	}
	tab=malloc(j+1);
	for(k=0;k<j;k++)
	{
		tab[k]=i/puissance_dix(j-k-1)%10+'0';
	}
	tab[j]='\0';
	kv->ptr=tab;
	if(kv->ptr==NULL) 
	{
		printf("j vaut %d\n",j);
		exit(5);
	}
	kv->len=j+1;
	return kv;
}
kv_datum ** creation_tableau_random(int n, int MAX) 
{
	int i;
	int nombre_aleatoire;
	kv_datum * kv;
	kv_datum ** tableau=malloc(sizeof( kv_datum *)*(n+1));
	for(i=0;i<=n;i++)
	{
		nombre_aleatoire = rand()%(MAX-0) +0;
		kv=transformation(nombre_aleatoire);
		tableau[i]=kv;
	}
	return tableau;

}

kv_datum * mot(int lenght_max)
{

	kv_datum *kv=malloc(sizeof( kv_datum));
	char *tab; 
	char lettre;
	char lettrestring[2];
	int lenght = rand()%(lenght_max+1);
	lettrestring[1]='\0';
	int i;
	tab= malloc(lenght+1);
	strcpy(tab, "");
	for (i = 0; i < lenght; ++i)
	{
		//srand(i);
//		printf("%d\n",rand());
		lettre='a'+(rand()%27);
		lettrestring[0]=lettre;
		strcat(tab, lettrestring);
	}
	kv->ptr=tab;
	kv->len=lenght+1;

	return kv;

}
kv_datum ** creation_tableau_random2(int n, int MAX) 
{
	int i;
	int nombre_aleatoire;
	kv_datum * kv;
	kv_datum ** tableau=malloc(sizeof( kv_datum *)*(n+1));
	for(i=0;i<=n;i++)
	{
		nombre_aleatoire = rand()%(MAX-0) +0;
		kv=mot(nombre_aleatoire);
		tableau[i]=kv;
	}
	return tableau;

}
void remplit_base(int n,KV *kv,kv_datum **tableau)
{
	int i;
	for(i=0;i<n;i++)
	{
		kv_put(kv,tableau[i],tableau[i+1]);
	}
	return ;	
}

//fonction qui supprime tous les clefs de valeur entre i et j
// la base est déjà ouverte
// retourne -1 si un des del n'a pas fonctionné sinon retourne 
int del (int i, int j, KV*kv,kv_datum **tableau)
{
	int test=0;
	int k;
	for(k=i;k<j;k++)
	{
		if(kv_del(kv,tableau[k])==-1)
			test=-1;
	}
	
	return test;	
}





void test_random(KV *kv, int taille, int taillefor,kv_datum **tableau)
{

	remplit_base(taille,kv,tableau);
	int taille_tab = 0, nb_put_reussie = taille, nb_del_reussie = 0;
	int nb_put_Nreussie = 0, nb_del_Nreussie = 0;
	int i,j;
	int tab_pos[1000];
	int choix;
	int booldp;
	int m;
	int k;
	int delpdp=0;
	for (k = 0; k < taillefor; k++)
	{		
		choix = rand()%(2-0) +0;	
		j = rand()%((taille_tab+1)  +0);
		if(j!=0&&j==taille_tab) 
			j--;
		if (choix == 1)
		{
			i = rand()%(taille-1) +0;
			if(i!=0 && i==taille_tab) i--;	
			booldp=0;
			if(kv_put(kv,tableau[i],tableau[j])== -1)
			{
				nb_put_Nreussie++;
			}
			else
			{
				for(m=0;m<taille_tab;m++)
				{
					if(tab_pos[m]==i)
					{
						delpdp++;
						booldp=1;
						break;
					}
				}
				if(booldp==0)
				{
					tab_pos[taille_tab]= i;
					taille_tab++;
					nb_put_reussie++;
				}
			}
			
		}
		else
		{
			if (taille_tab == 0)
			{
				continue;
			}

			if (kv_del(kv,tableau[tab_pos[j]])==-1)
			{
				
				nb_del_Nreussie++;
			}
			else
			{
				if (taille_tab == 1)
				{
					
					taille_tab--;
				}
				else
				{
					tab_pos[j] = tab_pos[taille_tab-1];
					taille_tab--;
					
				}
				nb_del_reussie++;
			}
		}
	}

	for(i=0;i<=1000;i++)
	{
		free(tableau[i]->ptr);
		free(tableau[i]);
	}
	free(tableau);
}



// affiche toute la base 
// la base est ouverte
void affiche_base(KV * kv)
{
	kv_datum key;
	kv_datum val;
	printf("------------------------------\n     CONTENU DE LA BASE :     \n------------------------------\n");
	kv_start(kv);
	key.ptr=NULL;
	val.ptr=NULL;
	while(kv_next(kv,&key,&val)==1)
	{
		
		printf("CLEF : longueur %u, valeur \n",key.len);
		write(1,key.ptr,key.len);
		printf("\nVal : longueur %u valeur \n",val.len);
		write(1,val.ptr,val.len);
		printf("\n");
		free(key.ptr);
		free(val.ptr);
		key.ptr=NULL;
		val.ptr=NULL;
	}	
	printf("------------------------------\n       FIN DE LA BASE        \n------------------------------\n");
}


// fonction qui stock des chaines de 0 dans la base.
int main(int argc, char* argv[])
{
	srand(time(NULL));
	int taille; // par défaut met 1000 valeurs
	alloc_t a; // par défaut met alloc à first fit
	int hidx=0;
	int taillemax;
	int nb_for;
	if(argc<8)
	{
		printf("usage %s : <nom de base> <methode d'allocation> ",argv[0]);
		printf("<mode> <hidx> <nb max> <nb boucle> <taille>\n");
		exit(1);
	}
	taillemax=atoi(argv[5]);
	a=atoi(argv[2]);
	hidx=atoi(argv[4]);
	nb_for=atoi(argv[6]);
	taille=atoi(argv[7]);
	if(argc==9) // lorsqu'on donne une graine pour pouvoir reproduire les test
		srand(atoi(argv[8])); 
	//tableau de random de int
	kv_datum **tableau = creation_tableau_random(taille, taillemax); 
	//tableau de random de mot 
	kv_datum **tableau2 = creation_tableau_random2(taille, 1500);
	
	KV*kv=kv_open(argv[1],argv[3],hidx,a);
	if(kv==NULL)
	{
		printf("erreur\n");
		exit(1);
	}
	test_random(kv, taille,nb_for,tableau);
	test_random(kv, taille,nb_for,tableau2);
	
 	if(kv_close(kv)==-1)
		printf("erreur\n");	
	
	return 0;
}

