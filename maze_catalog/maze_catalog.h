#ifndef MAZE_CATALOG_H
#define MAZE_CATALOG_H

#include "../maze_core/maze_core.h"

/* Satu entri katalog: identitas labirin dan file tempat ia disimpan. */
typedef struct {
    char name[32];
    char file[UTIL_MAX_PATH];
    int h;
    int w;
    int difficulty;
    int best_steps;
} MazeInfo;

/* Katalog labirin: array dinamis plus penanda apakah sudah terurut nama. */
typedef struct {
    MazeInfo *items;
    int count;
    int capacity;
    int sorted_by_name;
} MazeCatalog;

/* Muat katalog dari file; baris yang rusak dihitung ke bad_lines. */
Status maze_catalog_load(const char *path, MazeCatalog *cat, int *bad_lines);

/* Simpan katalog ke file dengan format yang sama seperti saat dimuat. */
Status maze_catalog_save(const char *path, const MazeCatalog *cat);

/* Tambah satu entri ke katalog; urutan namanya jadi tidak sah lagi. */
Status maze_catalog_add(MazeCatalog *cat, const MazeInfo *info);

/* Urutkan katalog berdasarkan ukuran sel atau kesulitan. */
void maze_catalog_sort_by_size(MazeCatalog *cat);
void maze_catalog_sort_by_difficulty(MazeCatalog *cat);
void maze_catalog_sort_by_name(MazeCatalog *cat);

/* Cari nama secara sequential atau binary; binary mengembalikan -2 kalau belum terurut nama. */
int maze_catalog_find_name_seq(const MazeCatalog *cat, const char *name);
int maze_catalog_find_name_binary(const MazeCatalog *cat, const char *name);

/* Melepas seluruh isi katalog. */
void maze_catalog_free(MazeCatalog *cat);

#endif
