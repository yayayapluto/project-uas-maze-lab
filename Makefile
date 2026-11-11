CC      ?= gcc
CFLAGS  ?= -std=c99 -Wall -Wextra -Wpedantic -g
LDLIBS  ?= -lm
SRC     := common/common.c maze_core.c maze_validate.c maze_catalog.c maze_gen.c \
           maze_solve.c maze_editor.c maze_bench.c maze_bench_report.c \
           maze_catalog_menu.c maze_bench_menu.c maze_lab.c maze_test.c \
           maze_test_extra.c maze_main.c
TARGET  := maze_lab
ASAN_TARGET := maze_lab_asan

.PHONY: all test asan clean

all: $(TARGET)

$(TARGET): $(SRC) common/common.h maze_core.h maze_catalog.h maze_gen.h maze_solve.h \
          maze_editor.h maze_bench.h maze_lab.h maze_test.h
	$(CC) $(CFLAGS) -o $@ $(SRC) $(LDLIBS)

test: $(TARGET)
	./$(TARGET) --test

asan: $(SRC)
	$(CC) $(CFLAGS) -fsanitize=address,undefined -o $(ASAN_TARGET) $(SRC) $(LDLIBS)

clean:
	rm -f $(TARGET) $(ASAN_TARGET) *.o common/*.o
