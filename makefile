# Compiler and flags
CC           := clang
CFLAGS       := -Wall -Wextra -Wpedantic -Wshadow -Wstrict-prototypes \
                 -Wmissing-prototypes -Wconversion -Wsign-conversion \
                 -Wfloat-equal -Wcast-align -Wcast-qual -Wwrite-strings \
                 -Wswitch-enum -Wunreachable-code -Wformat=2 -Wundef \
                 -Wpointer-arith -Wredundant-decls -Wmissing-declarations \
                 -Wbad-function-cast -Wvla -Wstrict-overflow=5 -Winline \
                 -Werror -std=c99 -O2 -D_POSIX_C_SOURCE=200112L

DEBUG_CFLAGS := -std=c99  -g

# Directories (no longer “debug”!)
BUILD_DIR    := build
DEBUG_DIR    := debug-build

# Sources, objects, and targets
SRC          := main.c term.c editor.c ds.c
OBJ          := $(SRC:%.c=$(BUILD_DIR)/%.o)
DEBUG_OBJ    := $(SRC:%.c=$(DEBUG_DIR)/%.o)
BIN          := $(BUILD_DIR)/porta
DEBUG_BIN    := $(DEBUG_DIR)/porta

.PHONY: all clean debug

# default build
all: $(BIN)

# quick “debug” build
debug: $(DEBUG_BIN)

run: $(BIN)
	./$(BIN)

# Link
$(BIN): $(OBJ) | $(BUILD_DIR)
	$(CC) $(OBJ) -o $(BIN)

$(DEBUG_BIN): $(DEBUG_OBJ) | $(DEBUG_DIR)
	$(CC) $(DEBUG_OBJ) -o $(DEBUG_BIN)

# Compile into build/ and debug-build/
$(BUILD_DIR)/%.o: %.c | $(BUILD_DIR)
	$(CC) $(CFLAGS)       -c $< -o $@

$(DEBUG_DIR)/%.o: %.c | $(DEBUG_DIR)
	$(CC) $(DEBUG_CFLAGS) -c $< -o $@

# ensure directories exist
$(BUILD_DIR) $(DEBUG_DIR):
	mkdir -p $@

clean:
	rm -rf $(BUILD_DIR) $(DEBUG_DIR)
