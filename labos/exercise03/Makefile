#-*-makefile-*-

# Compiler and flags
CC ?= cc
INCLUDE_DIR ?= include

# Additional compiler flags
CFLAGS += -std=gnu11 -O2 -Wall -Wextra -Wpedantic -Werror
CFLAGS += -g -ggdb

# Additional linker flags
LDFLAGS += -Lm -lm -pthread -lssl -lcrypto

# Sanitizer
# SANITIZERS += -fsanitize=address \
#               -fsanitize=pointer-compare \
#               -fsanitize=pointer-subtract \
#               -fsanitize=leak

SANITIZERS += -fsanitize=thread

SANITIZERS += -fsanitize=undefined
SANITIZERS += -fsanitize-address-use-after-scope
SANITIZERS += -fsanitize-undefined-trap-on-error

# CFLAGS += $(SANITIZERS)
# LDFLAGS += $(SANITIZERS)

# Additional include dirs
INCLUDE += -I$(INCLUDE_DIR)

# Target name
TARGET = main
TARGET_ALIASES = main

# Source and object files
SRC_DIR ?= src
BUILD_DIR ?= build

TARGET_LOCATION ?= $(BUILD_DIR)/$(TARGET)

# List of source files
SRC_FILES = $(wildcard $(SRC_DIR)/*.c)
HEADER_FILES = $(wildcard $(INCLUDE_DIR)/*.h)

# Generate the list of object files from source files
OBJ_FILES = $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(SRC_FILES))

#.SUFFIXES: .c .o .h

.PHONY: all
all: $(TARGET_LOCATION)

.PHONY: run
run: $(TARGET_LOCATION)
	@$(TARGET_LOCATION)

$(TARGET): $(TARGET_LOCATION)

$(TARGET_ALIASES): $(TARGET_LOCATION)
	@cp $(TARGET_LOCATION) $@

# Rule for building object files from source files
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) $(INCLUDE) -c $< -MMD -o $@

$(BUILD_DIR)/$(TARGET): $(OBJ_FILES)
	$(CC) $^ -o $@ $(LDFLAGS)

.PHONY: clean
clean:
	rm -rf $(BUILD_DIR)/
	rm -f $(TARGET_ALIASES)

#.PHONY: print
#print:
#	listing $(HEADER_FILES) $(SRC_FILES)

.PHONY: info
info:
	@echo "Compiler: $(CC)"
	@echo "Include Directory: $(INCLUDE_DIR)"
	@echo "Additional Compiler Flags: $(CFLAGS)"
	@echo "Additional Linker Flags: $(LDFLAGS)"
	@echo "Source Directory: $(SRC_DIR)"
	@echo "Build Directory: $(BUILD_DIR)"
	@echo "Source Files: $(SRC_FILES)"
	@echo "Header Files: $(HEADER_FILES)"
	@echo "Object Files: $(OBJ_FILES)"
	@echo "Executable Target: $(TARGET)"
	@echo "Target Aliases: $(TARGET_ALIASES)"


################################################################################
#LIBS = -lglut -lGLU -lGL -lGLw -lXt -lXext -lX11 -lm 
#
#SRCS = \
#	staaf.c lees.c
#
#OBJS = $(SRCS:.c=.o)
#
#.SUFFIXES:	.c .o .h
#
#.c.o:	
#	cc -c $< $(CFLAGS)
#
#
#staaf:  clean $(OBJS)
#	cc -o staaf $(CFLAGS) $(OBJS) $(LIBPATH) $(LIBS) 
#
#clean:
#	rm -f staaf $(OBJS) core
#
#print:
#	listing staaf.h $(SRCS)
