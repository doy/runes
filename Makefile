OUT      = runes
OBJ      = runes.o display.o xlib.o term.o events.o
CFLAGS  ?= -g -Wall -Wextra -Werror
LDFLAGS ?= -g -Wall -Wextra -Werror

build: $(OUT)

$(OUT): $(OBJ)
	$(CC) $(shell pkg-config --libs cairo-xlib libuv) $(LDFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(shell pkg-config --cflags cairo-xlib libuv) $(CFLAGS) -c -o $@ $^

clean:
	rm -f $(OUT) $(OBJ)

.PHONY: build clean
