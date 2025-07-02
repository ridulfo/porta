# Compiler and flags
CC           := clang
CFLAGS       := -Wall -Wextra -Wpedantic -Wshadow -Wstrict-prototypes \
                 -Wmissing-prototypes -Wconversion -Wsign-conversion \
                 -Wfloat-equal -Wcast-align -Wcast-qual -Wwrite-strings \
                 -Wswitch-enum -Wunreachable-code -Wformat=2 -Wundef \
                 -Wpointer-arith -Wredundant-decls -Wmissing-declarations \
                 -Wbad-function-cast -Wvla -Wstrict-overflow=5 -Winline \
                 -Werror -std=c99 -O2 -D_POSIX_C_SOURCE=200112L

DEBUG_CFLAGS := -std=c99 -g

# Directory layout
BUILD_DIR    := build
RELEASE_DIR  := $(BUILD_DIR)/release
DEBUG_DIR    := $(BUILD_DIR)/debug

# Sources, objects, binaries
SRC          := main.c term.c editor.c ds.c

RELEASE_OBJS := $(SRC:%.c=$(RELEASE_DIR)/%.o)
DEBUG_OBJS   := $(SRC:%.c=$(DEBUG_DIR)/%.o)

RELEASE_BIN  := $(RELEASE_DIR)/porta
DEBUG_BIN    := $(DEBUG_DIR)/porta

TEST_MODULES := ds
TESTS        := $(TEST_MODULES:%=$(DEBUG_DIR)/%_test)

# UI test components
PTY_TEST_OBJS    := $(DEBUG_DIR)/pty_test.o
UI_TEST_BIN      := $(DEBUG_DIR)/ui_test
SIMPLE_UI_TEST   := $(DEBUG_DIR)/simple_ui_test

.PHONY: all debug run clean test ui-test install

# default = release build
all: $(RELEASE_BIN)

debug: $(DEBUG_BIN)

run: all
	@./$(RELEASE_BIN)

install: all
	cp $(RELEASE_BIN) /usr/local/bin

$(RELEASE_BIN): $(RELEASE_OBJS) | $(RELEASE_DIR)
	$(CC) $^ -o $@

$(DEBUG_BIN): $(DEBUG_OBJS)   | $(DEBUG_DIR)
	$(CC) $^ -o $@

$(RELEASE_DIR)/%.o: %.c | $(RELEASE_DIR)
	$(CC) $(CFLAGS)       -c $< -o $@

$(DEBUG_DIR)/%.o: %.c | $(DEBUG_DIR)
	$(CC) $(DEBUG_CFLAGS) -c $< -o $@

$(DEBUG_DIR)/%_test: %.c %.h | $(DEBUG_DIR)
	$(CC) $(CFLAGS) -DPT_TEST -o $@ $<

test: $(TESTS)
	@echo
	@echo "==== Running tests ===="
	@for t in $(TESTS); do \
	  echo "-- $$t --"; \
	  $$t || exit 1; \
	done
	@echo "All tests passed."

# PTY test framework
$(PTY_TEST_OBJS): pty_test.c pty_test.h | $(DEBUG_DIR)
	$(CC) $(DEBUG_CFLAGS) -c pty_test.c -o $@

# UI test binaries
$(UI_TEST_BIN): ui_test.c $(PTY_TEST_OBJS) | $(DEBUG_DIR)
	$(CC) $(DEBUG_CFLAGS) ui_test.c $(PTY_TEST_OBJS) -lutil -o $@

$(SIMPLE_UI_TEST): simple_ui_test.c $(PTY_TEST_OBJS) | $(DEBUG_DIR)
	$(CC) $(DEBUG_CFLAGS) simple_ui_test.c $(PTY_TEST_OBJS) -lutil -o $@

ui-test: $(SIMPLE_UI_TEST) debug
	@echo
	@echo "==== Running UI tests ===="
	@./$(SIMPLE_UI_TEST)
	@echo "All UI tests passed."

$(RELEASE_DIR) $(DEBUG_DIR):
	mkdir -p $@

clean:
	rm -rf $(BUILD_DIR)
