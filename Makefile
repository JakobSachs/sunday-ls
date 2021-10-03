CC=clang
CFLAGS= -Wall -g
OBJ = src/ls.c

%.o: %.c 
	$(CC) -c -o $@ $< $(CFLAGS)

main: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS)
