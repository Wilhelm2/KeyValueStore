# WILHELM Daniel YAMAN Kendal


# Commence par put 10 000 valeurs
# puis en supprime ou en ajoute de façon aléatoire 5000 fois
gcc -Wall -Wextra -Werror -o main mainrandom.c kv.c
./main database FIRST_FIT "r+" 2 10000 5000 5000

# put à nouveau les même valeurs pour tester
./main database FIRST_FIT "r+" 2 10000 5000 5000

# put cette fois-ci 1 000 valeurs avec le mode w
./main database FIRST_FIT "r+" 2 10000 5000 5000

# put cette fois-ci 1 000 valeurs avec le mode w+

./main database FIRST_FIT "r+" 2 10000 5000 5000

rm database*
gcc -fprofile-arcs -ftest-coverage -o main maingcov.c kv.c
./main
gcov kv.c


rm database*
rm main
