OUT      = runes
OBJ      = runes.o display.o term.o parser.o window-xlib.o pty-unix.o
LIBS     = cairo cairo-xlib libuv
CFLAGS  ?= -g -Wall -Wextra -Werror
LDFLAGS ?= -g -Wall -Wextra -Werror

ALLCFLAGS  = $(shell pkg-config --cflags $(LIBS)) $(CFLAGS)
ALLLDFLAGS = $(shell pkg-config --libs $(LIBS)) $(LDFLAGS)

MAKEDEPEND = $(CC) $(ALLCFLAGS) -M -MP -MT '$@ $(@:%.o=.%.d)'

build: $(OUT)

$(OUT): $(OBJ)
	$(CC) $(ALLLDFLAGS) -o $@ $^

%.o: %.c
	@$(MAKEDEPEND) -o $(<:%.c=.%.d) $<
	$(CC) $(ALLCFLAGS) -c -o $@ $<

%.c: %.l
	$(LEX) -o $@ $<

clean:
	rm -f $(OUT) $(OBJ) $(OBJ:%.o=.%.d)

-include $(OBJ:%.o=.%.d)

.PHONY: build clean
