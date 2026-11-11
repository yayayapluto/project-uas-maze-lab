#include "maze_bench.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define REPORT_CLASSES 10
#define REPORT_MAX_BAR 40

typedef struct {
    char *buf;
    size_t len;
    size_t cap;
} BenchText;

static const char *const REPORT_GEN[2] = { "backtracker", "prim" };
static const char *const REPORT_SOLVER[3] = { "dfs", "bfs", "wall" };

/* Tambahkan teks terformat ke buffer laporan; buffer diperbesar kalau tidak muat. */
static Status bt_add(BenchText *t, const char *fmt, ...)
{
    va_list ap;
    int need;

    va_start(ap, fmt);
    need = vsnprintf(NULL, 0, fmt, ap);
    va_end(ap);
    if (need < 0) {
        return ST_BAD_FORMAT;
    }
    if (t->len + (size_t)need + 1 > t->cap) {
        size_t newcap = (t->cap == 0) ? 512 : t->cap;
        char *tmp;
        while (newcap < t->len + (size_t)need + 1) {
            newcap *= 2;
        }
        tmp = (char *)util_realloc(t->buf, newcap);
        if (tmp == NULL) {
            return ST_NO_MEMORY;
        }
        t->buf = tmp;
        t->cap = newcap;
    }
    va_start(ap, fmt);
    vsnprintf(t->buf + t->len, t->cap - t->len, fmt, ap);
    va_end(ap);
    t->len += (size_t)need;
    return ST_OK;
}

/* Melepas buffer laporan. */
static void bt_free(BenchText *t)
{
    util_free(t->buf);
    t->buf = NULL;
    t->len = 0;
    t->cap = 0;
}

/* Tulis judul dan kepala tabel ringkasan. */
static Status report_head(BenchText *t, int n, int h, int w, int loops)
{
    return bt_add(t, "=== RINGKASAN BENCHMARK (n=%d, ukuran %dx%d, loops=%d) ===\n", n, h, w, loops);
}

/* Tulis satu baris tabel untuk satu kombinasi generator-solver. */
static Status report_row(BenchText *t, const MazeBenchStat *st, const char *gen, const char *solver)
{
    if (st->has_length) {
        return bt_add(t, "%-12s %-8s %4d  %8d  %5.1f  %3d  %4d  %14.1f  %8.1f\n",
                      gen, solver, st->tries, st->ok, st->avg_length, st->min_length,
                      st->max_length, st->avg_visited, st->fail_pct);
    }
    return bt_add(t, "%-12s %-8s %4d  %8d  %5s  %3s  %4s  %14.1f  %8.1f\n",
                  gen, solver, st->tries, st->ok, "-", "-", "-", st->avg_visited, st->fail_pct);
}

/* Tulis semua baris ringkasan sesuai urutan generator dan solver. */
static Status report_table(BenchText *t, const MazeBenchRow *rows, int count)
{
    int g;
    int s;
    Status st;

    st = bt_add(t, "%-12s %-8s %4s  %8s  %5s  %3s  %4s  %14s  %8s\n",
                "Generator", "Solver", "Coba", "Berhasil", "Rata2", "Min", "Maks",
                "Rata2-dikunjungi", "Gagal(%)");
    for (g = 0; g < 2; g++) {
        for (s = 0; s < 3; s++) {
            MazeBenchStat st_row;
            st = maze_bench_stats(rows, count, REPORT_GEN[g], REPORT_SOLVER[s], &st_row);
            if (st != ST_OK) {
                return st;
            }
            st = report_row(t, &st_row, REPORT_GEN[g], REPORT_SOLVER[s]);
            if (st != ST_OK) {
                return st;
            }
        }
    }
    return ST_OK;
}

/* Hitung rentang dan jumlah kelas histogram panjang jalur BFS. */
static Status histogram_bounds(const MazeBenchRow *rows, int count, int *min_len,
                                int *max_len, int *classes)
{
    int i;
    int lo = 0;
    int hi = 0;
    int found = 0;

    for (i = 0; i < count; i++) {
        if (strcmp(rows[i].solver, "bfs") != 0 || !rows[i].found) {
            continue;
        }
        if (found == 0) {
            lo = rows[i].length;
            hi = rows[i].length;
            found = 1;
        } else {
            if (rows[i].length < lo) {
                lo = rows[i].length;
            }
            if (rows[i].length > hi) {
                hi = rows[i].length;
            }
        }
    }
    if (found == 0) {
        return ST_BAD_FORMAT;
    }
    *min_len = lo;
    *max_len = hi;
    *classes = (lo == hi) ? 1 : REPORT_CLASSES;
    return ST_OK;
}

/* Hitung banyaknya panjang jalur BFS per kelas histogram. */
static void histogram_counts(const MazeBenchRow *rows, int count, int min_len, int max_len,
                             int classes, int *counts)
{
    int i;
    int width = (max_len - min_len + classes) / classes;
    int k;

    if (width < 1) {
        width = 1;
    }
    for (k = 0; k < classes; k++) {
        counts[k] = 0;
    }
    for (i = 0; i < count; i++) {
        int low;
        int high;
        if (strcmp(rows[i].solver, "bfs") != 0 || !rows[i].found) {
            continue;
        }
        for (k = 0; k < classes; k++) {
            low = min_len + k * width;
            high = low + width - 1;
            if (k == classes - 1) {
                high = max_len;
            }
            if (rows[i].length >= low && rows[i].length <= high) {
                counts[k]++;
                break;
            }
        }
    }
}

/* Tulis satu kelas histogram beserta batang dan jumlahnya. */
static Status report_class(BenchText *t, int low, int high, int count, int scale)
{
    int bars = count / scale;
    int b;
    Status st = bt_add(t, "[%3d - %3d] ", low, high);

    for (b = 0; b < bars && st == ST_OK; b++) {
        st = bt_add(t, "#");
    }
    if (st != ST_OK) {
        return st;
    }
    return bt_add(t, " %d\n", count);
}

/* Tulis bagian histogram panjang jalur BFS. */
static Status report_histogram(BenchText *t, const MazeBenchRow *rows, int count)
{
    int counts[REPORT_CLASSES];
    int min_len;
    int max_len;
    int classes;
    int width;
    int max_count = 0;
    int scale = 1;
    int k;
    Status st;

    st = histogram_bounds(rows, count, &min_len, &max_len, &classes);
    if (st != ST_OK) {
        return ST_OK;
    }
    histogram_counts(rows, count, min_len, max_len, classes, counts);
    for (k = 0; k < classes; k++) {
        if (counts[k] > max_count) {
            max_count = counts[k];
        }
    }
    if (max_count > REPORT_MAX_BAR) {
        scale = max_count / REPORT_MAX_BAR + 1;
    }
    width = (max_len - min_len + classes) / classes;
    if (width < 1) {
        width = 1;
    }
    st = bt_add(t, "\nHistogram panjang jalur BFS\n");
    for (k = 0; k < classes && st == ST_OK; k++) {
        int low = min_len + k * width;
        int high = low + width - 1;
        if (k == classes - 1) {
            high = max_len;
        }
        st = report_class(t, low, high, counts[k], scale);
    }
    return st;
}

/* Susun teks laporan benchmark; teks ini dipakai baik untuk layar maupun file. */
static Status build_report(const MazeBenchRow *rows, int count, int n, int h, int w,
                           int loops, char **out, size_t *out_len)
{
    BenchText t;
    Status st;

    memset(&t, 0, sizeof t);
    st = report_head(&t, n, h, w, loops);
    if (st == ST_OK) {
        st = report_table(&t, rows, count);
    }
    if (st == ST_OK) {
        st = report_histogram(&t, rows, count);
    }
    if (st != ST_OK) {
        bt_free(&t);
        return st;
    }
    *out = t.buf;
    *out_len = t.len;
    return ST_OK;
}

/* Tampilkan ringkasan benchmark beserta histogram ke layar. */
void maze_bench_summary(const MazeBenchRow *rows, int count, int n, int h, int w, int loops)
{
    char *text = NULL;
    size_t len = 0;

    if (build_report(rows, count, n, h, w, loops, &text, &len) != ST_OK) {
        printf("Ringkasan benchmark tidak dapat disusun.\n");
        return;
    }
    fwrite(text, 1, len, stdout);
    util_free(text);
}

/* Tulis laporan benchmark ke file teks; isinya sama dengan ringkasan di layar. */
Status maze_bench_write_report(const char *path, const MazeBenchRow *rows, int count,
                                int n, int h, int w, int loops)
{
    char *text = NULL;
    size_t len = 0;
    Status st;

    st = build_report(rows, count, n, h, w, loops, &text, &len);
    if (st != ST_OK) {
        return st;
    }
    st = util_write_file(path, text, len);
    util_free(text);
    return st;
}
