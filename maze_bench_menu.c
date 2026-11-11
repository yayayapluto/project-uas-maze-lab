#include "maze_lab.h"

#include <stdio.h>
#include <string.h>

#define BENCH_SEED_MAX 2147483647
#define BENCH_LOOPS_MAX 9999

/* Membaca dimensi ganjil antara 5 dan 61 untuk benchmark. */
static int read_odd_dims(int *h, int *w)
{
    for (;;) {
        if (util_read_int("Tinggi (5-61, ganjil): ", 5, 61, h) != ST_OK) {
            return 0;
        }
        if (util_read_int("Lebar (5-61, ganjil): ", 5, 61, w) != ST_OK) {
            return 0;
        }
        if (*h % 2 == 1 && *w % 2 == 1) {
            return 1;
        }
        printf("Ukuran harus ganjil.\n");
    }
}

/* Menjalankan urutan baris benchmark menurut sel dikunjungi. */
static void sort_by_visited(MazeBenchRow *rows, int count)
{
    maze_bench_sort_by_visited(rows, count);
    printf("Baris diurutkan menurut sel dikunjungi.\n");
}

/* Menjalankan urutan baris benchmark menurut panjang jalur. */
static void sort_by_length(MazeBenchRow *rows, int count)
{
    maze_bench_sort_by_length(rows, count);
    printf("Baris diurutkan menurut panjang jalur.\n");
}

/* Mencari baris benchmark dengan seed yang diketik pengguna. */
static void find_seed(const MazeBenchRow *rows, int count)
{
    int seed;
    int idx;

    if (util_read_int("Seed: ", 0, BENCH_SEED_MAX, &seed) != ST_OK) {
        return;
    }
    idx = maze_bench_find_seed(rows, count, (unsigned)seed);
    if (idx < 0) {
        printf("Seed tidak ditemukan.\n");
        return;
    }
    printf("Seed %d ditemukan pada baris %d (%s, %s).\n", seed, idx + 1,
           rows[idx].gen, rows[idx].solver);
}

/* Menulis laporan benchmark ke path yang diketik pengguna. */
static void write_report(const MazeBenchRow *rows, int count, int n, int h, int w, int loops)
{
    char path[UTIL_MAX_PATH];
    Status st;

    if (util_read_line("Path berkas laporan: ", path, sizeof path) != ST_OK) {
        return;
    }
    util_trim(path);
    if (path[0] == '\0') {
        printf("Path tidak boleh kosong.\n");
        return;
    }
    st = maze_bench_write_report(path, rows, count, n, h, w, loops);
    printf("%s\n", util_status_message(st));
}

/* Menjalankan submenu benchmark sampai pengguna memilih selesai. */
void maze_menu_bench(void)
{
    MazeBenchRow *rows = NULL;
    int count = 0;
    int n;
    int seed;
    int h;
    int w;
    int loops;
    int running = 1;
    Status st;

    if (util_read_int("Jumlah labirin (1-1000): ", 1, 1000, &n) != ST_OK) {
        return;
    }
    if (util_read_int("Seed dasar (0-2147483647): ", 0, BENCH_SEED_MAX, &seed) != ST_OK) {
        return;
    }
    if (!read_odd_dims(&h, &w)) {
        return;
    }
    if (util_read_int("Loops (0-9999): ", 0, BENCH_LOOPS_MAX, &loops) != ST_OK) {
        return;
    }
    st = maze_bench_run(n, (unsigned)seed, h, w, loops, &rows, &count);
    if (st != ST_OK) {
        printf("%s\n", util_status_message(st));
        util_free(rows);
        return;
    }
    maze_bench_summary(rows, count, n, h, w, loops);
    while (running) {
        int choice;
        printf("\n1. Urut sel dikunjungi\n");
        printf("2. Urut panjang jalur\n");
        printf("3. Cari seed\n");
        printf("4. Tulis laporan\n");
        printf("0. Selesai\n");
        if (util_read_int("Pilihan: ", 0, 4, &choice) != ST_OK) {
            break;
        }
        switch (choice) {
        case 1:
            sort_by_visited(rows, count);
            break;
        case 2:
            sort_by_length(rows, count);
            break;
        case 3:
            find_seed(rows, count);
            break;
        case 4:
            write_report(rows, count, n, h, w, loops);
            break;
        default:
            running = 0;
            break;
        }
    }
    util_free(rows);
}
