PREFIX ?= $(HOME)/.local
BIN_DIR ?= $(PREFIX)/bin

TARGET_EXEC ?= crt
BUILD_DIR ?= ./build
SRC_DIR	?= ./src

SRCS := $(shell find $(SRC_DIR) -name *.cpp -or -name *.c)
OBJS := $(SRCS:$(SRC_DIR)/%=$(BUILD_DIR)/%.o)

# Common flags
CPPFLAGS += -Wall -Wextra

ifeq ($(RELEASE),1)
	CPPFLAGS += -O3
else
	CPPFLAGS += -ggdb -Og
endif

# C specific flags
CFLAGS +=

# Linker flags
LDFLAGS +=

# Executable
$(BUILD_DIR)/$(TARGET_EXEC): $(OBJS)
	$(CXX) $(OBJS) -o $@ $(LDFLAGS)

# C source
$(BUILD_DIR)/%.c.o: $(SRC_DIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

.PHONY: all run install clean

all: $(BUILD_DIR)/$(TARGET_EXEC)

run: $(BUILD_DIR)/$(TARGET_EXEC)
	./$<

install: $(BUILD_DIR)/$(TARGET_EXEC)
	install -Dt $(BIN_DIR) $<

clean:
	rm -r $(BUILD_DIR)
