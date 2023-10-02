CFLAGS	:= -Wall -Wextra -Werror -g
CC		:= gcc
SRC_DIR := src
OBJ_DIR := obj
BIN_DIR := bin

SRCS    := $(wildcard $(SRC_DIR)/*.c)
OBJS    := $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(SRCS))
EXE 	:= $(BIN_DIR)/main

.PHONY: all clean


FILES= $(OBJ_DIR)/hashFunctions.o $(OBJ_DIR)/kvInitialization.o $(OBJ_DIR)/commonFunctions.o $(OBJ_DIR)/kv.o $(OBJ_DIR)/slotAllocations.o 

$(OBJ_DIR):
	mkdir $@
$(BIN_DIR):
	mkdir $@

all: random
	
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

random: $(OBJS) | $(BIN_DIR)
	ls $(OBJS)
	gcc -o $(EXE) $(CFLAGS) $(FILES) $(OBJ_DIR)/mainmotrandom.o	

zero:
	gcc -o $(EXE) $(FILES) $(OBJ_DIR)/mainzero.o	

int:
	gcc -o $(EXE) $(FILES) $(OBJ_DIR)/mainint.o	


clean:
	rm *.o
	rm main
