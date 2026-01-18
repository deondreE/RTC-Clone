CC = gcc
ASM = nasm
CFLAGS = -Wall -Wextra -O2 -Iinclude `sdl2-config --cflags`
ASMFLAGS = -f elf64
LDFLAGS = `sdl2-config --libs` -lm

# Directories
SRC_DIR = src
BUILD_DIR = build
ASSETS_DIR = assets

C_SOURCES = $(shell find $(SRC_DIR) -name '*.c')
ASM_SOURCES = $(shell find $(SRC_DIR) -name '*.asm')

C_OBJECTS = $(C_SOURCES:$(SRC_DIR)/%.c=$(BUILD_DIR)/%.o)
ASM_OBJECTS = $(ASM_SOURCES:$(SRC_DIR)/%.asm=$(BUILD_DIR)/%.o)

TARGET = rct-clone

.PHONY: all clean run dirs

all: dirs $(TARGET)

dirs:
	@mkdir -p $(BUILD_DIR)/game $(BUILD_DIR)/render $(BUILD_DIR)/ui $(BUILD_DIR)/core
	@mkdir -p $(ASSETS_DIR)/sprites

$(TARGET): $(C_OBJECTS) $(ASM_OBJECTS)
	$(CC) $^ -o $@ $(LDFLAGS)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.asm
	$(ASM) $(ASMFLAGS) $< -o $@

clean:
	rm -rf $(BUILD_DIR) $(TARGET)

run: all
	./$(TARGET)
