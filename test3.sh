# WILHELM Daniel YAMAN Kendal
echo "troisieme test"
gcc -Wall -Wextra -Werror -o main mainrandom.c kv.c
# test les temps des différents méthodes de hache
# pour entrer 10 000 clefs constituées de nombre entre 0 et 5000 random
# et pour en supprimer de nouveau 5000
(time ./main database FIRST_FIT "r+" 0 5000 5000 5000 ) 2> fichier
user1=$(cat fichier| sed -n "1p" | cut -d " " -f1 | sed 's/[a-z]//g')
sys1=$(cat fichier| sed -n "1p" | cut -d " " -f2 | sed 's/[a-z]//g')
echo $(echo " $user1 +$sys1" | bc)

rm data*
(time ./main database FIRST_FIT "r+" 2 5000 5000 5000 ) 2> fichier
user2=$(cat fichier| sed -n "1p" | cut -d " " -f1 | sed 's/[a-z]//g')
sys2=$(cat fichier| sed -n "1p" | cut -d " " -f2 | sed 's/[a-z]//g')
echo $(echo " $user2 +$sys2" | bc)
rm data*
(time ./main database FIRST_FIT "r+" 3 5000 5000 5000 ) 2> fichier
user3=$(cat fichier| sed -n "1p" | cut -d " " -f1 | sed 's/[a-z]//g')
sys3=$(cat fichier| sed -n "1p" | cut -d " " -f2 | sed 's/[a-z]//g')
echo $(echo " $user2 +$sys2" | bc)
rm data*
rm fichier
