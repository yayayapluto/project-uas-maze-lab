#include "maze_bench.h"

#include "../maze_gen/maze_gen.h"
#include "../maze_solve/maze_solve.h"

#include <stdlib.h>
#include <string.h>

#define BENCH_MAX_N 1000
#define BENCH_COMBOS 6

static const char *const BENCH_GEN[BENCH_COMBOS / 2] = { "backtracker", "prim" };
static const char *const BENCH_SOLVER[3] = { "dfs", "bfs", "wall" };

/* Menjalankan satu solver dan mengisi baris benchmark yang sesuai. */
static Status run_solver(const Maze *m, const char *solver, MazeBenchRow *row)
{
    MazeSolveResult res;
    MazeCell *path = NULL;
    Status st;

    if (strcmp(solver, "dfs") == 0) {
        st = maze_solve_dfs(m, &res, &path);
    } else if (strcmp(solver, "bfs") == 0) {
        st = maze_solve_bfs(m, &res, &path);
    } else {
        st = maze_solve_wall(m, &res, &path);
    }
    if (st != ST_OK) {
        util_free(path);
        return st;
    }
    row->found = res.found;
    row->length = res.length;
    row->visited = res.visited;
    util_free(path);
    return ST_OK;
}

/* Menjalankan satu labirin untuk satu generator dan ketiga solver. */
static Status run_maze(MazeBenchRow *rows, int *filled, unsigned seed, int gen_idx,
                       int h, int w, int loops)
{
    Maze maze;
    int s;
    Status st;

    memset(&maze, 0, sizeof maze);
    st = (gen_idx == 0) ? maze_gen_backtracker(&maze, h, w, seed, loops)
                        : maze_gen_prim(&maze, h, w, seed, loops);
    if (st != ST_OK) {
        maze_destroy(&maze);
        return st;
    }
    for (s = 0; s < 3; s++) {
        MazeBenchRow *row = &rows[*filled];
        memset(row, 0, sizeof *row);
        row->seed = seed;
        strcpy(row->gen, BENCH_GEN[gen_idx]);
        strcpy(row->solver, BENCH_SOLVER[s]);
        st = run_solver(&maze, row->solver, row);
        if (st != ST_OK) {
            maze_destroy(&maze);
            return st;
        }
        (*filled)++;
    }
    maze_destroy(&maze);
    return ST_OK;
}

/* Menjalankan benchmark untuk n seed berurutan dan dua generator. */
Status maze_bench_run(int n, unsigned base_seed, int h, int w, int loops,
                      MazeBenchRow **rows, int *count)
{
    MazeBenchRow *arr;
    int total;
    int filled = 0;
    int i;
    int g;
    Status st;

    if (rows == NULL || count == NULL) {
        return ST_BAD_INPUT;
    }
    if (n < 1 || n > BENCH_MAX_N) {
        return ST_OUT_OF_RANGE;
    }
    total = n * 2 * 3;
    arr = (MazeBenchRow *)util_calloc((size_t)total, sizeof(MazeBenchRow));
    if (arr == NULL) {
        return ST_NO_MEMORY;
    }
    for (i = 0; i < n; i++) {
        unsigned seed = base_seed + (unsigned)i;
        for (g = 0; g < 2; g++) {
            st = run_maze(arr, &filled, seed, g, h, w, loops);
            if (st != ST_OK) {
                util_free(arr);
                *rows = NULL;
                *count = 0;
                return st;
            }
        }
    }
    *rows = arr;
    *count = filled;
    return ST_OK;
}

/* Mengurutkan baris menurut sel dikunjungi dengan Bubble Sort. */
void maze_bench_sort_by_visited(MazeBenchRow *rows, int count)
{
    int i;
    int j;

    if (rows == NULL) {
        return;
    }
    for (i = 0; i < count - 1; i++) {
        for (j = 0; j < count - 1 - i; j++) {
            if (rows[j].visited > rows[j + 1].visited) {
                MazeBenchRow tmp = rows[j];
                rows[j] = rows[j + 1];
                rows[j + 1] = tmp;
            }
        }
    }
}

/* Mengurutkan baris menurut panjang jalur dengan Selection Sort. */
void maze_bench_sort_by_length(MazeBenchRow *rows, int count)
{
    int i;
    int j;

    if (rows == NULL) {
        return;
    }
    for (i = 0; i < count - 1; i++) {
        int best = i;
        for (j = i + 1; j < count; j++) {
            if (rows[j].length < rows[best].length) {
                best = j;
            }
        }
        if (best != i) {
            MazeBenchRow tmp = rows[i];
            rows[i] = rows[best];
            rows[best] = tmp;
        }
    }
}

/* Mencari baris dengan seed tertentu secara sequential. */
int maze_bench_find_seed(const MazeBenchRow *rows, int count, unsigned seed)
{
    int i;

    if (rows == NULL) {
        return -1;
    }
    for (i = 0; i < count; i++) {
        if (rows[i].seed == seed) {
            return i;
        }
    }
    return -1;
}

/* Menghitung statistik satu kombinasi generator dan solver dari nama baris. */
Status maze_bench_stats(const MazeBenchRow *rows, int count, const char *gen,
                        const char *solver, MazeBenchStat *out)
{
    int i;
    int ok = 0;
    int sum_len = 0;
    int min_len = 0;
    int max_len = 0;
    long sum_visited = 0;

    if (rows == NULL || out == NULL || gen == NULL || solver == NULL) {
        return ST_BAD_INPUT;
    }
    memset(out, 0, sizeof *out);
    for (i = 0; i < count; i++) {
        if (strcmp(rows[i].gen, gen) != 0 || strcmp(rows[i].solver, solver) != 0) {
            continue;
        }
        out->tries++;
        sum_visited += rows[i].visited;
        if (rows[i].found) {
            ok++;
            sum_len += rows[i].length;
            if (out->has_length == 0) {
                min_len = rows[i].length;
                max_len = rows[i].length;
                out->has_length = 1;
            } else {
                if (rows[i].length < min_len) {
                    min_len = rows[i].length;
                }
                if (rows[i].length > max_len) {
                    max_len = rows[i].length;
                }
            }
        }
    }
    out->ok = ok;
    if (out->tries > 0) {
        out->avg_visited = (double)sum_visited / (double)out->tries;
        out->fail_pct = (double)(out->tries - ok) * 100.0 / (double)out->tries;
    }
    if (ok > 0) {
        out->avg_length = (double)sum_len / (double)ok;
        out->min_length = min_len;
        out->max_length = max_len;
    }
    return ST_OK;
}
