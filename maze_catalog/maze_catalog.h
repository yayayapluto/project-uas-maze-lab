#ifndef MAZE_CATALOG_H
#define MAZE_CATALOG_H

#include "../maze_core/maze_core.h"

/* Satu entri katalog: identitas labirin dan berkas tempatnya disimpan. */
typedef struct {
    char name[32];
    char file[UTIL_MAX_PATH];
    int h;
    int w;
    int difficulty;
    int best_steps;
} MazeInfo;

/* Katalog labirin dengan array dinamis dan penanda urutan nama. */
typedef struct {
    MazeInfo *items;
    int count;
    int capacity;
    int sorted_by_name;
} MazeCatalog;

/* Memuat katalog dari berkas; baris rusak dihitung pada bad_lines. */
Status maze_catalog_load(const char *path, MazeCatalog *cat, int *bad_lines);

/* Menyimpan katalog ke berkas dengan format yang sama seperti berkas muat. */
Status maze_catalog_save(const char *path, const MazeCatalog *cat);

/* Menambah satu entri ke katalog dan mengubah urutan nama menjadi belum terurut. */
Status maze_catalog_add(MazeCatalog *cat, const MazeInfo *info);

/* Mengurutkan katalog menurut ukuran sel dan menurut kesulitan. */
void maze_catalog_sort_by_size(MazeCatalog *cat);
void maze_catalog_sort_by_difficulty(MazeCatalog *cat);
void maze_catalog_sort_by_name(MazeCatalog *cat);

/* Mencari nama secara sequential dan binary; binary mengembalikan -2 bila belum terurut. */
int maze_catalog_find_name_seq(const MazeCatalog *cat, const char *name);
int maze_catalog_find_name_binary(const MazeCatalog *cat, const char *name);

/* Melepas seluruh isi katalog. */
void maze_catalog_free(MazeCatalog *cat);

#endif
