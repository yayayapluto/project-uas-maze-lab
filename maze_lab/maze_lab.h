#ifndef MAZE_LAB_H
#define MAZE_LAB_H

#include "../maze_bench/maze_bench.h"
#include "../maze_catalog/maze_catalog.h"
#include "../maze_core/maze_core.h"

/* Jalankan menu utama Maze Lab sampai pengguna memilih keluar. */
void run_maze_lab(void);

/* Jalankan seluruh uji otomatis; kembalikan jumlah yang gagal. */
int test_maze_lab(void);

/* Submenu katalog: lihat, urutkan, cari, tambah, buka, simpan. */
void maze_menu_catalog(MazeCatalog *cat, int *loaded, Maze *active);

/* Submenu benchmark: jalankan eksperimen, lalu urutkan, cari seed, dan tulis laporan. */
void maze_menu_bench(void);

#endif
