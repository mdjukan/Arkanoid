CC = gcc
CFLAGS = -lraylib -lGL -lm -lpthread -ldl -lrt -lX11

X : 
	$(CC) main.c $(CFLAGS)

