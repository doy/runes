OUT      = runes
OBJ      = runes.o display.o term.o events.o window-xlib.o
LIBS     = cairo cairo-xlib libuv
CFLAGS  ?= -g -Wall -Wextra -Werror
LDFLAGS ?= -g -Wall -Wextra -Werror

build: $(OUT)

$(OUT): $(OBJ)
	$(CC) $(shell pkg-config --libs $(LIBS)) $(LDFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(shell pkg-config --cflags $(LIBS)) $(CFLAGS) -c -o $@ $^

clean:
	rm -f $(OUT) $(OBJ)

.PHONY: build clean
