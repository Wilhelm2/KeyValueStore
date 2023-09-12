CFLAGS= -Wall -Wextra -Werror -g
CC= gcc
FILES= hashFunctions.c kvInitialization.c commonFunctions.c kv.c slotAllocations.c 

random:
	$(CC) $(CFLAGS) -c $(FILES)
	$(CC) $(CFLAGS) -c mainmotrandom.c
	gcc -o main $(CFLAGS) $(FILES) mainmotrandom.o	

zero:
	$(CC) $(CFLAGS) -c $(FILES)
	$(CC) $(CFLAGS) -c mainzero.c
	gcc -o main $(FILES) mainzero.o	

int:
	$(CC) $(CFLAGS) -c $(FILES)
	$(CC) $(CFLAGS) -c mainint.c
	gcc -o main $(FILES) mainint.o	



