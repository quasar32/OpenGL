CC = gcc
CFLAGS = -Wall -g
CFLAGS = -Ilib/cglm/include -Ilib/glad/include -Ilib/glfw/include -Ilib/stb
LDFLAGS = -lopengl32 -mwindows -mconsole
LDFLAGS += lib/glad/src/glad.o lib/cglm/libcglm.a lib/glfw/src/libglfw3.a 

SRC = $(wildcard *.c) 
OBJ = $(SRC:.c=.o)
BIN = bin

all: libs game

dirs:
	mkdir -p ./$(BIN)

libs:
	cd lib/cglm && cmake . -DCGLM_STATIC=ON && mingw32-make
	cd lib/glad && $(CC) -o src/glad.o -Iinclude -c src/glad.c
	cd lib/glfw && cmake . && mingw32-make

game: $(OBJ)
	$(CC) $^ $(LDFLAGS)

run: all
	$(BIN)

%.o: %.c
	$(CC) -c $< $(CFLAGS)

