OUT      = runes
OBJ      = runes.o display.o term.o parser.o window-xlib.o pty-unix.o
LIBS     = cairo cairo-xlib libuv
CFLAGS  ?= -g -Wall -Wextra -Werror
LDFLAGS ?= -g -Wall -Wextra -Werror

GENERATED = parser.c

build: $(OUT)

$(OUT): $(OBJ)
	$(CC) $(shell pkg-config --libs $(LIBS)) $(LDFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(shell pkg-config --cflags $(LIBS)) $(CFLAGS) -c -o $@ $^

%.c: %.l
	$(LEX) -o $@ $^

clean:
	rm -f $(OUT) $(OBJ) $(GENERATED)

.PHONY: build clean
