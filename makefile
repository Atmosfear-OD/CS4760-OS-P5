CC = gcc
CFLAG = -g
TAR1 = oss
TAR2 = user
M_OBJ = oss.o
U_OBJ = user.o
OBJ = matrix.o
OBJ2 = queue.o

all: $(TAR1) $(TAR2)

$(TAR1): $(M_OBJ) $(OBJ) $(OBJ2)
	$(CC) $(CFLAG) -o $(TAR1) $(M_OBJ) $(OBJ) $(OBJ2)

$(TAR2): $(U_OBJ) $(OBJ) $(OBJ2)
	$(CC) $(CFLAG) -o $(TAR2) $(U_OBJ) $(OBJ) $(OBJ2)

$(M_OBJ): oss.c
	$(CC) $(CFLAG) -c oss.c 

$(U_OBJ): user.c
	$(CC) $(CFLAG) -c user.c

$(OBJ): matrix.c
	$(CC) $(CFLAG) -c matrix.c

$(OBJ2): queue.c
	$(CC) $(CFLAG) -c queue.c

clean:
	rm -rf *.o *.log $(TAR1) $(TAR2)
