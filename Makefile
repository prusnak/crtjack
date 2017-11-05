NAME=crtjack
CC=gcc
CFLAGS=$(shell pkg-config --cflags jack sdl2) -Wall
LDFLAGS=
LIBS=$(shell pkg-config --libs jack sdl2)
OBJ=main.o

$(NAME): $(OBJ)
	$(CC) $(LDFLAGS) $(LIBS) $(OBJ) -o $(NAME)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(NAME) $(OBJ)
