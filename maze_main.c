#include "maze_lab/maze_lab.h"

#include <stdio.h>
#include <string.h>

/* Menjalankan executable mandiri: tanpa argumen membuka menu, --test menjalankan uji. */
int main(int argc, char **argv)
{
    if (argc == 2 && strcmp(argv[1], "--test") == 0) {
        return test_maze_lab();
    }
    if (argc == 1) {
        run_maze_lab();
        return 0;
    }
    printf("Pemakaian: %s            membuka menu Maze Lab\n", argv[0]);
    printf("           %s --test     menjalankan seluruh uji otomatis\n", argv[0]);
    return 1;
}
