# Compiler and flags
CC           := gcc
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
SRC          := main.c term.c editor.c ds.c render.c

RELEASE_OBJS := $(SRC:%.c=$(RELEASE_DIR)/%.o)
DEBUG_OBJS   := $(SRC:%.c=$(DEBUG_DIR)/%.o)

RELEASE_BIN  := $(RELEASE_DIR)/porta
DEBUG_BIN    := $(DEBUG_DIR)/porta

TEST_MODULES := ds
TESTS        := $(TEST_MODULES:%=$(DEBUG_DIR)/%_test)

.PHONY: all debug run clean test install

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

$(RELEASE_DIR) $(DEBUG_DIR):
	mkdir -p $@

clean:
	rm -rf $(BUILD_DIR)
