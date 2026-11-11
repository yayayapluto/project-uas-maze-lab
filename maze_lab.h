#ifndef MAZE_LAB_H
#define MAZE_LAB_H

#include "maze_bench.h"
#include "maze_catalog.h"
#include "maze_core.h"

/* Menjalankan menu utama Maze Lab sampai pengguna memilih keluar. */
void run_maze_lab(void);

/* Menjalankan seluruh uji otomatis dan mengembalikan jumlah kegagalan. */
int test_maze_lab(void);

/* Submenu katalog: lihat, urut, cari, tambah, buka, dan simpan. */
void maze_menu_catalog(MazeCatalog *cat, int *loaded, Maze *active);

/* Submenu benchmark: menjalankan eksperimen lalu offering urutan, cari, dan laporan. */
void maze_menu_bench(void);

#endif
