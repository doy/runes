OUT      = runes
DOUT     = runesd
COUT     = runesc
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
DOBJ     = $(BUILD)runesd.o \
	   $(BUILD)display.o \
	   $(BUILD)term.o \
	   $(BUILD)config.o \
	   $(BUILD)window-xlib.o \
	   $(BUILD)pty-unix.o \
	   $(BUILD)loop.o \
	   $(BUILD)util.o \
	   $(BUILD)socket.o
COBJ     = $(BUILD)runesc.o \
	   $(BUILD)util.o
LIBS     = cairo cairo-xlib libuv pangocairo
CFLAGS  ?= -g -Wall -Wextra -Werror
LDFLAGS ?= -g -Wall -Wextra -Werror

ALLCFLAGS  = $(shell pkg-config --cflags $(LIBS)) -Ilibvt100/src $(CFLAGS)
ALLLDFLAGS = $(shell pkg-config --libs $(LIBS)) $(LDFLAGS)

MAKEDEPEND = $(CC) $(ALLCFLAGS) -M -MP -MT '$@ $(@:$(BUILD)%.o=$(BUILD).%.d)'

all: $(OUT) $(DOUT) $(COUT) ## Build all of the targets

run: $(OUT) ## Build and run the standalone runes terminal
	@./$(OUT)

run-daemon: $(DOUT) $(COUT) ## Build and run the runes daemon
	@./$(DOUT)

$(OUT): $(OBJ) libvt100/libvt100.a
	$(CC) $(ALLLDFLAGS) -o $@ $^

$(DOUT): $(DOBJ) libvt100/libvt100.a
	$(CC) $(ALLLDFLAGS) -o $@ $^

$(COUT): $(COBJ)
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

clean: ## Remove build files
	cd libvt100 && make clean
	rm -f $(OUT) $(OBJ) $(OBJ:$(BUILD)%.o=$(BUILD).%.d) $(DOUT) $(DOBJ) $(DOBJ:$(BUILD)%.o=$(BUILD).%.d) $(COUT) $(COBJ) $(COBJ:$(BUILD)%.o=$(BUILD).%.d)
	@rmdir -p $(BUILD) > /dev/null 2>&1

help:
	@grep -HE '^[a-zA-Z_-]+:.*?## .*$$' $(MAKEFILE_LIST) | sort | awk 'BEGIN {FS = ":|##"}; {printf "\033[36m%-20s\033[0m %s\n", $$2, $$4}'

-include $(OBJ:$(BUILD)%.o=$(BUILD).%.d)

.PHONY: all run run-daemon clean help
