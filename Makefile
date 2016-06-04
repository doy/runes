OUT      = runes
DOUT     = runesd
COUT     = runesc
BUILD    = build/
SRC      = src/
BASE_OBJ = $(BUILD)util.o
TERM_OBJ = $(BUILD)display.o \
	   $(BUILD)term.o \
	   $(BUILD)config.o \
	   $(BUILD)window-xlib.o \
	   $(BUILD)window-backend-xlib.o \
	   $(BUILD)pty-unix.o \
	   $(BUILD)loop.o
SOCK_OBJ = $(BUILD)socket.o \
	   $(BUILD)protocol.o
OBJ      = $(BUILD)runes.o \
	   $(BASE_OBJ) \
	   $(TERM_OBJ)
DOBJ     = $(BUILD)runesd.o \
	   $(BUILD)daemon.o \
	   $(BASE_OBJ) \
	   $(TERM_OBJ) \
	   $(SOCK_OBJ)
COBJ     = $(BUILD)runesc.o \
	   $(BASE_OBJ) \
	   $(SOCK_OBJ)
LIBS     = cairo cairo-xlib libevent pangocairo
OPT     ?= -g
CFLAGS  ?= $(OPT) -Wall -Wextra -Werror -std=c1x -D_XOPEN_SOURCE=600
LDFLAGS ?= $(OPT)

ALLCFLAGS  = $(shell pkg-config --cflags $(LIBS)) -Ilibvt100/src $(CFLAGS)
ALLLDFLAGS = $(shell pkg-config --libs $(LIBS)) $(LDFLAGS)

MAKEDEPEND = $(CC) $(ALLCFLAGS) -M -MP -MT '$@ $(@:$(BUILD)%.o=$(BUILD).%.d)'

all: $(OUT) $(DOUT) $(COUT) ## Build all of the targets

release: ## Build optimized binaries
	$(MAKE) clean
	$(MAKE) OPT=-O2
	strip $(OUT) $(DOUT) $(COUT)

run: $(OUT) ## Build and run the standalone runes terminal
	@./$(OUT)

run-daemon: $(DOUT) $(COUT) ## Build and run the runes daemon
	@./$(DOUT)

$(OUT): $(OBJ) libvt100/libvt100.a
	$(CC) -o $@ $^ $(ALLLDFLAGS)

$(DOUT): $(DOBJ) libvt100/libvt100.a
	$(CC) -o $@ $^ $(ALLLDFLAGS)

$(COUT): $(COBJ)
	$(CC) -o $@ $^ $(ALLLDFLAGS)

libvt100/libvt100.a: make-libvt100

make-libvt100:
	@if ! $(MAKE) -q -C libvt100 static; then $(MAKE) -C libvt100 static && MAKELEVEL=$(echo "${MAKELEVEL}-1" | bc) exec $(MAKE) $(MAKECMDGOALS); fi

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
	@rmdir -p $(BUILD) > /dev/null 2>&1 || true

help:
	@grep -HE '^[a-zA-Z_-]+:.*?## .*$$' $(MAKEFILE_LIST) | sort | awk 'BEGIN {FS = ":|##"}; {printf "\033[36m%-20s\033[0m %s\n", $$2, $$4}'

-include $(OBJ:$(BUILD)%.o=$(BUILD).%.d)

.PHONY: all run run-daemon clean help make-libvt100
