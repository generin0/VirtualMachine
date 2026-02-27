CC = gcc
CFLAGS = -std=c11 -O2 -Wall -Wextra -Wpedantic
TARGET = vasm_compiler.exe

SRC_DIR = src
INC_DIR = include

SRCS = $(SRC_DIR)/main.c      \
       $(SRC_DIR)/error.c     \
       $(SRC_DIR)/assembler.c \
       $(SRC_DIR)/disasm.c    \
       $(SRC_DIR)/dump.c

OBJS = $(SRCS:.c=.o)

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

$(SRC_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -I$(INC_DIR) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)