# Commond variable
CC = /usr/bin/env gcc

WORK_DIR := .
SRC_DIR = $(WORK_DIR) $(WORK_DIR)/tools $(WORK_DIR)/3rd-party
OUTPUT_DIR := ./out
VPATH= $(SRC_DIR)

MODULE_NAME = lsvc
LIB_TARGET = $(OUTPUT_DIR)/lib$(MODULE_NAME).so
EXE_TARGET = $(OUTPUT_DIR)/$(MODULE_NAME)
TARGETS = $(LIB_TARGET) $(EXE_TARGET)

# Compiler flags to enable all warnings & debug info
CFLAGS := -std=c99 -fasm -D_GNU_SOURCE -Wall -Werror -g -O0

# Lib flags
LIB_CFLAGS := $(CFLAGS) -fPIC -shared -rdynamic -Wno-discarded-qualifiers \
				-Wno-address-of-packed-member \
				-Wno-pointer-to-int-cast -Wno-pointer-sign -Wno-int-conversion 
				
LIB_LDFLAGS := -fPIC -L$(OUTPUT_DIR) -l$(MODULE_NAME) -pthread -lrt

# C source code files that are required
INCDIR = $(addprefix -I, $(SRC_DIR))
CSRC := $(foreach dir, $(SRC_DIR), $(wildcard $(dir)/*.c))

# C obj files generated from source code
COBJ := $(addprefix $(OUTPUT_DIR)/., $(notdir $(CSRC:%.c=%.o)))
LIB_COBJ := $(filter-out $(OUTPUT_DIR)/.main.o, $(COBJ))

# Build rule for the main program
all: build_prepare $(TARGETS)

build_prepare:
	mkdir -p $(OUTPUT_DIR)

$(OUTPUT_DIR)/.%.o: %.c
	$(CC) $(LIB_CFLAGS) -c -o $@ $< $(INCDIR)

$(LIB_TARGET): $(LIB_COBJ)
	$(CC) $(LIB_CFLAGS) -o $@ $^
$(EXE_TARGET): $(OUTPUT_DIR)/.main.o $(LIB_TARGET)
	$(CC) $(CFLAGS) -o $@ $^ $(LIB_LDFLAGS)

clean: 
	rm -rf $(COBJ) $(LIB_TARGET) $(EXE_TARGET)