OUT      = runes
BUILD    = build/
SRC      = src/
OBJ      = $(BUILD)runes.o \
	   $(BUILD)display.o \
	   $(BUILD)term.o \
	   $(BUILD)config.o \
	   $(BUILD)window-xlib.o \
	   $(BUILD)pty-unix.o \
	   $(BUILD)loop.o \
	   $(BUILD)util.o
LIBS     = cairo cairo-xlib libuv pangocairo
CFLAGS  ?= -g -Wall -Wextra -Werror
LDFLAGS ?= -g -Wall -Wextra -Werror

ALLCFLAGS  = $(shell pkg-config --cflags $(LIBS)) -Ilibvt100/src $(CFLAGS)
ALLLDFLAGS = $(shell pkg-config --libs $(LIBS)) $(LDFLAGS)

MAKEDEPEND = $(CC) $(ALLCFLAGS) -M -MP -MT '$@ $(@:$(BUILD)%.o=$(BUILD).%.d)'

build: $(OUT)

$(OUT): $(OBJ) libvt100/libvt100.a
	$(CC) $(ALLLDFLAGS) -o $@ $^

libvt100/libvt100.a:
	cd libvt100 && make static

$(BUILD)%.o: $(SRC)%.c
	@mkdir -p $(BUILD)
	@$(MAKEDEPEND) -o $(<:$(SRC)%.c=$(BUILD).%.d) $<
	$(CC) $(ALLCFLAGS) -c -o $@ $<

$(SRC)screen.c: $(SRC)parser.h

$(SRC)%.c: $(SRC)%.l
	$(LEX) -o $@ $<

$(SRC)%.h: $(SRC)%.l
	$(LEX) --header-file=$(<:.l=.h) -o /dev/null $<

clean:
	cd libvt100 && make clean
	rm -f $(OUT) $(OBJ) $(OBJ:$(BUILD)%.o=$(BUILD).%.d)
	@rmdir -p $(BUILD) > /dev/null 2>&1

-include $(OBJ:$(BUILD)%.o=$(BUILD).%.d)

.PHONY: build clean
