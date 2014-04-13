OUT      = runes
OBJ      = runes.o display.o term.o parser.o window-xlib.o pty-unix.o
LIBS     = cairo cairo-xlib libuv
CFLAGS  ?= -g -Wall -Wextra -Werror
LDFLAGS ?= -g -Wall -Wextra -Werror

GENERATED = parser.c

build: $(OUT)

$(OUT): $(OBJ)
	$(CC) $(shell pkg-config --libs $(LIBS)) $(LDFLAGS) -o $@ $^

parser.o: parser.c
	$(CC) $(shell pkg-config --cflags $(LIBS)) $(CFLAGS) -Wno-unused-parameter -Wno-unused-function -Wno-unused-variable -Wno-unused-value -c -o $@ $^

%.o: %.c
	$(CC) $(shell pkg-config --cflags $(LIBS)) $(CFLAGS) -c -o $@ $^

%.c: %.l
	$(LEX) -o $@ $^

clean:
	rm -f $(OUT) $(OBJ) $(GENERATED)

.PHONY: build clean
