#ifndef MAZE_BENCH_H
#define MAZE_BENCH_H

#include "maze_core.h"

/* Satu baris hasil benchmark untuk kombinasi generator dan solver. */
typedef struct {
    unsigned seed;
    char gen[16];
    char solver[16];
    int found;
    int length;
    int visited;
} MazeBenchRow;

/* Statistik satu kombinasi generator dan solver. */
typedef struct {
    int tries;
    int ok;
    double avg_length;
    int min_length;
    int max_length;
    double avg_visited;
    double fail_pct;
    int has_length;
} MazeBenchStat;

/* Menjalankan benchmark dan mengisi baris hasil; baris dilepas pemanggil dengan util_free. */
Status maze_bench_run(int n, unsigned base_seed, int h, int w, int loops,
                      MazeBenchRow **rows, int *count);

/* Menampilkan ringkasan benchmark beserta histogram panjang jalur BFS. */
void maze_bench_summary(const MazeBenchRow *rows, int count, int n, int h, int w, int loops);

/* Mengurutkan baris menurut sel dikunjungi dan menurut panjang jalur. */
void maze_bench_sort_by_visited(MazeBenchRow *rows, int count);
void maze_bench_sort_by_length(MazeBenchRow *rows, int count);

/* Mencari baris dengan seed tertentu secara sequential. */
int maze_bench_find_seed(const MazeBenchRow *rows, int count, unsigned seed);

/* Menghitung statistik satu kombinasi generator dan solver. */
Status maze_bench_stats(const MazeBenchRow *rows, int count, const char *gen,
                        const char *solver, MazeBenchStat *out);

/* Menulis laporan benchmark ke berkas; isi sama dengan ringkasan di layar. */
Status maze_bench_write_report(const char *path, const MazeBenchRow *rows, int count,
                                int n, int h, int w, int loops);

#endif
