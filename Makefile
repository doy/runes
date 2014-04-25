OUT      = runes
BUILD    = build/
SRC      = src/
OBJ      = $(BUILD)runes.o \
	   $(BUILD)display.o \
	   $(BUILD)term.o \
	   $(BUILD)parser.o \
	   $(BUILD)screen.o \
	   $(BUILD)config.o \
	   $(BUILD)window-xlib.o \
	   $(BUILD)pty-unix.o
LIBS     = cairo cairo-xlib libuv pangocairo
CFLAGS  ?= -g -Wall -Wextra -Werror
LDFLAGS ?= -g -Wall -Wextra -Werror

ALLCFLAGS  = $(shell pkg-config --cflags $(LIBS)) $(CFLAGS)
ALLLDFLAGS = $(shell pkg-config --libs $(LIBS)) $(LDFLAGS)

MAKEDEPEND = $(CC) $(ALLCFLAGS) -M -MP -MT '$@ $(@:$(BUILD)%.o=$(BUILD).%.d)'

build: $(OUT)

$(OUT): $(OBJ)
	$(CC) $(ALLLDFLAGS) -o $@ $^

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
	rm -f $(OUT) $(OBJ) $(OBJ:$(BUILD)%.o=$(BUILD).%.d)
	@rmdir -p $(BUILD) > /dev/null 2>&1

-include $(OBJ:$(BUILD)%.o=$(BUILD).%.d)

.PHONY: build clean
