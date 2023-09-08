#WILHELM Daniel YAMAN Kendal

# Lance les fonctions de tests avec les différentes fonctions de hachage

echo "Premier test"
gcc -Wall -Wextra -Werror -o main mainmotrandom.c kv.c slotAllocations.c
# test les temps des différents méthodes de hache
# pour entrer 50 000 clefs + valeurs de taille comprise entre 0 et 10
# ainsi que pour en supprimer de nouveau 25 000 clefs
(time ./main database 40000 10 FIRST_FIT "r+" 0) 2> fichier
user1=$(cat fichier| sed -n "1p" | cut -d " " -f1 | sed 's/[a-z]//g')
sys1=$(cat fichier| sed -n "1p" | cut -d " " -f2 | sed 's/[a-z]//g')
echo "hachage par defaut $(echo " $user1 +$sys1" | bc)"

rm data*
(time ./main database 40000 10 FIRST_FIT "r+" 2) 2> fichier
user2=$(cat fichier| sed -n "1p" | cut -d " " -f1 | sed 's/[a-z]//g')
sys2=$(cat fichier| sed -n "1p" | cut -d " " -f2 | sed 's/[a-z]//g')
echo "seconde fonction de hachage $(echo " $user2 +$sys2" | bc)"
rm data*
(time ./main database 40000 10 FIRST_FIT "r+" 3) 2> fichier 
user3=$(cat fichier| sed -n "1p" | cut -d " " -f1 | sed 's/[a-z]//g')
sys3=$(cat fichier| sed -n "1p" | cut -d " " -f2 | sed 's/[a-z]//g')
echo "troisieme fonction de hachage $(echo " $user2 +$sys2" | bc)"
rm data*
rm fichier

echo "Second test"
gcc -Wall -Wextra -Werror -o main mainzero.c kv.c slotAllocations.c
# test les temps des différents méthodes de hache
# pour entrer 4 000 clefs + valeurs de taille comprise entre 0 et 10
# ainsi que pour en supprimer de nouveau 5 000 clefs
(time ./main database 4000 FIRST_FIT "r+" 0) 2> fichier
user1=$(cat fichier| sed -n "1p" | cut -d " " -f1 | sed 's/[a-z]//g')
sys1=$(cat fichier| sed -n "1p" | cut -d " " -f2 | sed 's/[a-z]//g')
echo " hachage par defaut $(echo " $user1 +$sys1" | bc)"

rm data*
(time ./main database 4000 FIRST_FIT "r+" 2) 2> fichier
user2=$(cat fichier| sed -n "1p" | cut -d " " -f1 | sed 's/[a-z]//g')
sys2=$(cat fichier| sed -n "1p" | cut -d " " -f2 | sed 's/[a-z]//g')
echo "seconde fonction de hachage $(echo " $user2 +$sys2" | bc)"
rm data*
(time ./main database 4000 FIRST_FIT "r+" 3) 2> fichier 
user3=$(cat fichier| sed -n "1p" | cut -d " " -f1 | sed 's/[a-z]//g')
sys3=$(cat fichier| sed -n "1p" | cut -d " " -f2 | sed 's/[a-z]//g')
echo "troisieme fonction de hachage $(echo " $user2 +$sys2" | bc)"
rm data*
rm fichier


echo "Troisieme test"
gcc -Wall -Wextra -Werror -o main mainrandom.c kv.c slotAllocations.c
# test les temps des différents méthodes de hache
# pour entrer 10 000 clefs constituées de nombre entre 0 et 5000 random
# puis pour entrer 10 000 mots de longueur max 10 random
# et pour en supprimer 10 000 d'entre elles
(time ./main database FIRST_FIT "r+" 0 10000 5000 5000 1) 2> fichier
user1=$(cat fichier| sed -n "1p" | cut -d " " -f1 | sed 's/[a-z]//g')
sys1=$(cat fichier| sed -n "1p" | cut -d " " -f2 | sed 's/[a-z]//g')
echo " hachage par defaut $(echo " $user1 +$sys1" | bc)"

rm data*
(time ./main database FIRST_FIT "r+" 2 10000 5000 5000 1) 2> fichier
user2=$(cat fichier| sed -n "1p" | cut -d " " -f1 | sed 's/[a-z]//g')
sys2=$(cat fichier| sed -n "1p" | cut -d " " -f2 | sed 's/[a-z]//g')
echo "seconde fonction de hachage $(echo " $user2 +$sys2" | bc)"
rm data*
(time ./main database FIRST_FIT "r+" 3 10000 5000 5000 1) 2> fichier
user3=$(cat fichier| sed -n "1p" | cut -d " " -f1 | sed 's/[a-z]//g')
sys3=$(cat fichier| sed -n "1p" | cut -d " " -f2 | sed 's/[a-z]//g')
echo "troisieme fonction de hachage $(echo " $user2 +$sys2" | bc)"
rm data*
rm fichier

echo "Quatrieme test"
gcc -Wall -Wextra -Werror -o main mainint.c kv.c slotAllocations.c
# test les temps des différents méthodes de hache
# pour entrer 10 000 clefs constituées de nombre entre 0 et 5000
# et pour en supprimer de nouveau 5000
(time ./main database 10000 FIRST_FIT "r+" 0 ) 2> fichier
user1=$(cat fichier| sed -n "1p" | cut -d " " -f1 | sed 's/[a-z]//g')
sys1=$(cat fichier| sed -n "1p" | cut -d " " -f2 | sed 's/[a-z]//g')
echo " hachage par defaut $(echo " $user1 +$sys1" | bc)"

rm data*
(time ./main database 10000 FIRST_FIT "r+" 2 ) 2> fichier
user2=$(cat fichier| sed -n "1p" | cut -d " " -f1 | sed 's/[a-z]//g')
sys2=$(cat fichier| sed -n "1p" | cut -d " " -f2 | sed 's/[a-z]//g')
echo "seconde fonction de hachage $(echo " $user2 +$sys2" | bc)"
rm data*
(time ./main database 10000 FIRST_FIT "r+" 3 ) 2> fichier
user3=$(cat fichier| sed -n "1p" | cut -d " " -f1 | sed 's/[a-z]//g')
sys3=$(cat fichier| sed -n "1p" | cut -d " " -f2 | sed 's/[a-z]//g')
echo "troisieme fonction de hachage $(echo " $user2 +$sys2" | bc)"
rm data*
rm fichier

rm main

