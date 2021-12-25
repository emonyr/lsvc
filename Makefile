# Commond variable
CC = /usr/bin/env gcc
SRC_DIR = .
OUTPUT_DIR := .

MODULE_NAME = lsvc
LIB_TARGET = $(OUTPUT_DIR)/lib$(MODULE_NAME).so
EXE_TARGET = $(OUTPUT_DIR)/$(MODULE_NAME)
TARGETS = $(LIB_TARGET) $(EXE_TARGET)

# Compiler flags to enable all warnings & debug info
CFLAGS := -Wall -Werror -g -O0

# Lib flags
LIB_CFLAGS := -fPIC -shared -Wall -rdynamic -Wno-discarded-qualifiers \
				-Wno-address-of-packed-member -Wno-incompatible-pointer-types \
				-Wno-pointer-to-int-cast -Wno-pointer-sign -Wno-int-conversion 
				
LIB_LDFLAGS := -fPIC -L$(OUTPUT_DIR) -l$(MODULE_NAME) -lpthread -lrt

# C source code files that are required
INCDIR = -I$(SRC_DIR) -I$(SRC_DIR)/tools
CSRC := ${wildcard $(SRC_DIR)/*.c} ${wildcard $(SRC_DIR)/tools/*.c}

# C obj files generated from source code
COBJ := $(CSRC:%.c=%.o)
LIB_COBJ := $(filter-out $(SRC_DIR)/main.o, $(COBJ))

# Build rule for the main program
all: $(TARGETS)

%.o:%.c
	$(CC) $(LIB_CFLAGS) -c -o $@ $< $(INCDIR)

$(LIB_TARGET):$(LIB_COBJ)
	$(CC) $(LIB_CFLAGS) -o $@ $^
$(EXE_TARGET): $(SRC_DIR)/main.o $(LIB_TARGET)
	$(CC) $(CFLAGS) -o $@ $^ $(LIB_LDFLAGS)

clean: 
	rm -rf $(COBJ) $(LIB_TARGET) $(EXE_TARGET)