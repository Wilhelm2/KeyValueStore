# WILHELM Daniel YAMAN Kendal


# appel de la fonction mainradom qui va faire des puts et des dels de
# façon aléatoire. On lui fournit une graine pour qu'elle effectue
# les même opérations dans les 3 tests

echo "Test des fonctions d'allocations"
gcc -Wall -Wextra -Werror -o main mainallocfirst.c kv.c
echo "FIRST FIT"
echo "Première exécution"
(time ./main database 15000 500  "r+" 2) 2> fichier
user1=$(cat fichier| sed -n "1p" | cut -d " " -f1 | sed 's/[a-z]//g')
sys1=$(cat fichier| sed -n "1p" | cut -d " " -f2 | sed 's/[a-z]//g')

echo "Seconde exécution"
(time ./main database 15000 500  "r+" 2 5) 2> fichier
user2=$(cat fichier| sed -n "1p" | cut -d " " -f1 | sed 's/[a-z]//g')
sys2=$(cat fichier| sed -n "1p" | cut -d " " -f2 | sed 's/[a-z]//g')
echo "$(echo " $user1 +$sys1+$user2 +$sys2" | bc) secondes"
echo "$(ls -l | grep "database.kv" )"
rm data*

echo "WORST FIT"
echo "Première exécution"
gcc -Wall -Wextra -Werror -o main mainallocworst.c kv.c
(time ./main database 15000 500  "r+" 2) 2> fichier2
user1=$(cat fichier| sed -n "1p" | cut -d " " -f1 | sed 's/[a-z]//g')
sys1=$(cat fichier| sed -n "1p" | cut -d " " -f2 | sed 's/[a-z]//g')

echo "Seconde exécution"
(time ./main database 15000 500 "r+" 2 5) 2> fichier2
user2=$(cat fichier| sed -n "1p" | cut -d " " -f1 | sed 's/[a-z]//g')
sys2=$(cat fichier| sed -n "1p" | cut -d " " -f2 | sed 's/[a-z]//g')
echo "WORST FIT $(echo " $user1 +$sys1+$user2 +$sys2" | bc) secondes"
echo "$(ls -l | grep "database.kv" )"
rm data*

echo "BEST FIT"
echo "Première exécution"
gcc -Wall -Wextra -Werror -o main mainallocbest.c kv.c
(time ./main database 15000 500 "r+" 2) 2> fichier3
user1=$(cat fichier| sed -n "1p" | cut -d " " -f1 | sed 's/[a-z]//g')
sys1=$(cat fichier| sed -n "1p" | cut -d " " -f2 | sed 's/[a-z]//g')

echo "Seconde exécution"
(time ./main database 15000 500 "r+" 2 5) 2> fichier3
user3=$(cat fichier| sed -n "1p" | cut -d " " -f1 | sed 's/[a-z]//g')
sys3=$(cat fichier| sed -n "1p" | cut -d " " -f2 | sed 's/[a-z]//g')
echo "BEST FIT $(echo " $user1 +$sys1+$user3 +$sys3" | bc) secondes"
echo "$(ls -l | grep "database.kv" )"
rm data*

rm main
rm fichier
rm fichier2
rm fichier3
