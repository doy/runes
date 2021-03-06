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

ifndef VERBOSE
QUIET_CC  = @echo "  CC  $@";
QUIET_LD  = @echo "  LD  $@";
endif

all: $(OUT) $(DOUT) $(COUT) ## Build all of the targets

release: ## Build optimized binaries
	$(MAKE) clean
	$(MAKE) OPT=-O2
	strip $(OUT) $(DOUT) $(COUT)

debug: CFLAGS += -DRUNES_PROGRAM_NAME='"runes-debug"'
debug: $(OUT) $(DOUT) $(COUT) ## Like 'all', but with a different window class and socket path

release-profile: OPT = -O2 -pg
release-profile: $(OUT) $(DOUT) $(COUT) ## Like 'release', but with profiling enabled

profile: CFLAGS += -DRUNES_PROGRAM_NAME='"runes-debug"'
profile: OPT = -g -pg
profile: $(OUT) $(DOUT) $(COUT) ## Like 'debug', but with profiling enabled

run: $(OUT) ## Build and run the standalone runes terminal
	@./$(OUT)

run-daemon: $(DOUT) $(COUT) ## Build and run the runes daemon
	@./$(DOUT)

$(OUT): $(OBJ) libvt100/libvt100.a
	$(QUIET_LD)$(CC) -o $@ $^ $(ALLLDFLAGS)

$(DOUT): $(DOBJ) libvt100/libvt100.a
	$(QUIET_LD)$(CC) -o $@ $^ $(ALLLDFLAGS)

$(COUT): $(COBJ)
	$(QUIET_LD)$(CC) -o $@ $^ $(ALLLDFLAGS)

libvt100/libvt100.a::
	@if ! $(MAKE) -q -C libvt100 static; then $(MAKE) -C libvt100 static OPT="$(OPT)" && MAKELEVEL=$(echo "${MAKELEVEL}-1" | bc) exec $(MAKE) $(MAKECMDGOALS); fi

$(BUILD)%.o: $(SRC)%.c | $(BUILD)
	@$(MAKEDEPEND) -o $(<:$(SRC)%.c=$(BUILD).%.d) $<
	$(QUIET_CC)$(CC) $(ALLCFLAGS) -c -o $@ $<

$(BUILD):
	@mkdir -p $(BUILD)

clean: ## Remove build files
	cd libvt100 && make clean
	rm -f $(OUT) $(OBJ) $(OBJ:$(BUILD)%.o=$(BUILD).%.d) $(DOUT) $(DOBJ) $(DOBJ:$(BUILD)%.o=$(BUILD).%.d) $(COUT) $(COBJ) $(COBJ:$(BUILD)%.o=$(BUILD).%.d)
	@rmdir -p $(BUILD) > /dev/null 2>&1 || true

help: ## Display this help
	@grep -HE '^[a-zA-Z_-]+:.*?## .*$$' $(MAKEFILE_LIST) | sort | awk 'BEGIN {FS = ":|##"}; {printf "\033[36m%-16s\033[0m %s\n", $$2, $$4}'

-include $(OBJ:$(BUILD)%.o=$(BUILD).%.d)
-include $(DOBJ:$(BUILD)%.o=$(BUILD).%.d)
-include $(COBJ:$(BUILD)%.o=$(BUILD).%.d)

.PHONY: all run run-daemon clean help
