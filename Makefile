CC      ?= gcc
CFLAGS  ?= -std=c99 -Wall -Wextra -Wpedantic -g
LDLIBS  ?= -lm
SRC     := common/common.c \
           maze_core/maze_core.c maze_core/maze_validate.c \
           maze_gen/maze_gen.c \
           maze_solve/maze_solve.c \
           maze_editor/maze_editor.c maze_editor/maze_editor_cmd.c \
           maze_catalog/maze_catalog.c \
           maze_bench/maze_bench.c maze_bench/maze_bench_report.c \
           maze_lab/maze_lab.c maze_lab/maze_catalog_menu.c \
           maze_lab/maze_bench_menu.c \
           maze_test/maze_test.c maze_test/maze_test_extra.c \
           ui/ui.c \
           maze_main.c
HDR     := common/common.h maze_core/maze_core.h maze_catalog/maze_catalog.h \
           maze_gen/maze_gen.h maze_solve/maze_solve.h \
           maze_editor/maze_editor.h maze_editor/maze_editor_cmd.h \
           maze_bench/maze_bench.h maze_lab/maze_lab.h maze_test/maze_test.h ui/ui.h
TARGET  := maze_lab
ASAN_TARGET := maze_lab_asan

ifeq ($(OS),Windows_NT)
TARGET     := maze_lab.exe
ASAN_TARGET := maze_lab_asan.exe
endif

.PHONY: all test asan clean

all: $(TARGET)

$(TARGET): $(SRC) $(HDR)
	$(CC) $(CFLAGS) -o $@ $(SRC) $(LDLIBS)

test: $(TARGET)
ifeq ($(OS),Windows_NT)
	$(TARGET) --test
else
	./$(TARGET) --test
endif

asan: $(SRC)
	$(CC) $(CFLAGS) -fsanitize=address,undefined -o $(ASAN_TARGET) $(SRC) $(LDLIBS)

clean:
ifeq ($(OS),Windows_NT)
	-cmd /c "del /q $(TARGET) $(ASAN_TARGET) *.o common\*.o 2>nul"
else
	rm -f $(TARGET) $(ASAN_TARGET) *.o common/*.o
endif


