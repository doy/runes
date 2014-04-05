OUT      = runes
OBJ      = runes.o xlib.o main.o
CFLAGS  ?= -g
LDFLAGS ?= -g

build: $(OUT)

$(OUT): $(OBJ)
	$(CC) $(shell pkg-config --libs cairo-xlib) $(LDFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(shell pkg-config --cflags cairo-xlib) $(CFLAGS) -c -o $@ $^

clean:
	rm -f $(OUT) $(OBJ)

.PHONY: build clean
