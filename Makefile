CC = gcc
CFLAGS = -Wall -Werror -DCOLOR
EXECUTABLE = 320sh
LIB_DIR = lib/
SRC_DIR = src/
INCLUDE_DIR = include/

EXECUTABLE_SRC = $(EXECUTABLE).c
SRC_C = $(wildcard $(SRC_DIR)*.c)
SRC_OBJS=$(SRC_C:.c=.o)
LIB_OBJS = $(wildcard $(LIB_DIR)*.o)

all: $(EXECUTABLE)

gdb_debug: CFLAGS += -g
gdb_debug: $(EXECUTABLE)

debug_print: CFLAGS += -DDEBUG -g
debug_print: $(EXECUTABLE)

$(EXECUTABLE): $(SRC_OBJS) $(LIB_OBJS) $(EXECUTABLE_SRC)
	$(CC) $(CFLAGS) -I $(INCLUDE_DIR) $^ -o $@

.c.o:
	$(CC) $(CFLAGS) -I $(INCLUDE_DIR) -c $< -o $@

.PHONY: clean
clean:
	rm -f $(SRC_OBJS)
