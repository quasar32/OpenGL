CC = gcc
CFLAGS = -Iglad/include -Icglm/include -Wall -g
LDFLAGS = glfw/libglfw3.a cglm/build/libcglm.dll.a -lopengl32 -mwindows -mconsole

SRC = $(wildcard glad/src/*.c) $(wildcard *.c) 
OBJ = $(SRC:.c=.o)

game: $(OBJ)
	$(CC) $^ $(LDFLAGS)

%.o: %.c
	$(CC) -c $< $(CFLAGS)

