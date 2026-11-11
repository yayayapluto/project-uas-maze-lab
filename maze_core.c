#include "maze_core.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Mengambil satu baris dari buffer dan menggeser kursor ke baris berikutnya. */
static char *take_line(char **cur)
{
    char *start = *cur;
    char *nl;

    if (start == NULL || *start == '\0') {
        return NULL;
    }
    nl = strchr(start, '\n');
    if (nl != NULL) {
        *nl = '\0';
        *cur = nl + 1;
    } else {
        *cur = start + strlen(start);
    }
    start[strcspn(start, "\r")] = '\0';
    return start;
}

/* Mengurai dua bilangan bulat dari satu baris; sisa baris harus kosong. */
static Status parse_two_ints(const char *s, int *a, int *b)
{
    char *end = NULL;
    long v1;
    long v2;

    v1 = strtol(s, &end, 10);
    if (end == s) {
        return ST_BAD_FORMAT;
    }
    while (isspace((unsigned char)*end)) {
        end++;
    }
    if (*end == '\0') {
        return ST_BAD_FORMAT;
    }
    s = end;
    v2 = strtol(s, &end, 10);
    if (end == s) {
        return ST_BAD_FORMAT;
    }
    while (isspace((unsigned char)*end)) {
        end++;
    }
    if (*end != '\0') {
        return ST_BAD_FORMAT;
    }
    *a = (int)v1;
    *b = (int)v2;
    return ST_OK;
}

/* Menyatakan karakter sel merupakan karakter yang sah dalam berkas labirin. */
static int valid_cell_char(char ch)
{
    return ch == '#' || ch == '.' || ch == 'S' || ch == 'E' || ch == '*';
}

/* Memeriksa seluruh sel tepi adalah dinding. */
static int border_ok(const char **rows, int h, int w)
{
    int r;
    int c;

    for (c = 0; c < w; c++) {
        if (rows[0][c] != '#' || rows[h - 1][c] != '#') {
            return 0;
        }
    }
    for (r = 0; r < h; r++) {
        if (rows[r][0] != '#' || rows[r][w - 1] != '#') {
            return 0;
        }
    }
    return 1;
}

/* Menghitung jumlah S dan E pada baris labirin. */
static void count_markers(const char **rows, int h, int w, int *n_s, int *n_e)
{
    int r;
    int c;

    *n_s = 0;
    *n_e = 0;
    for (r = 0; r < h; r++) {
        for (c = 0; c < w; c++) {
            if (rows[r][c] == 'S') {
                (*n_s)++;
            } else if (rows[r][c] == 'E') {
                (*n_e)++;
            }
        }
    }
}

/* Membuat labirin baru berukuran h x w dengan seluruh sel berisi dinding. */
Status maze_create(Maze *m, int h, int w)
{
    char **rows;
    int r;
    int i;

    if (m == NULL) {
        return ST_BAD_INPUT;
    }
    if (h < 5 || h > 61 || w < 5 || w > 61) {
        return ST_OUT_OF_RANGE;
    }
    rows = (char **)util_calloc((size_t)h, sizeof(char *));
    if (rows == NULL) {
        return ST_NO_MEMORY;
    }
    for (r = 0; r < h; r++) {
        rows[r] = (char *)util_calloc((size_t)w, sizeof(char));
        if (rows[r] == NULL) {
            for (i = 0; i < r; i++) {
                util_free(rows[i]);
            }
            util_free(rows);
            return ST_NO_MEMORY;
        }
        memset(rows[r], '#', (size_t)w);
    }
    m->grid = rows;
    m->h = h;
    m->w = w;
    m->start.r = 1;
    m->start.c = 1;
    m->end.r = h - 2;
    m->end.c = w - 2;
    return ST_OK;
}

/* Melepas tiap baris grid, lalu array baris, lalu mengosongkan labirin. */
void maze_destroy(Maze *m)
{
    int r;

    if (m == NULL) {
        return;
    }
    if (m->grid != NULL) {
        for (r = 0; r < m->h; r++) {
            util_free(m->grid[r]);
        }
        util_free(m->grid);
    }
    m->grid = NULL;
    m->h = 0;
    m->w = 0;
    m->start.r = 0;
    m->start.c = 0;
    m->end.r = 0;
    m->end.c = 0;
}

/* Menyalin grid, titik awal, dan titik tujuan ke labirin tujuan. */
Status maze_copy(const Maze *src, Maze *dst)
{
    Maze tmp;
    int r;
    Status st;

    if (src == NULL || dst == NULL || src->grid == NULL) {
        return ST_BAD_INPUT;
    }
    memset(&tmp, 0, sizeof tmp);
    st = maze_create(&tmp, src->h, src->w);
    if (st != ST_OK) {
        return st;
    }
    for (r = 0; r < src->h; r++) {
        memcpy(tmp.grid[r], src->grid[r], (size_t)src->w);
    }
    tmp.start = src->start;
    tmp.end = src->end;
    maze_destroy(dst);
    *dst = tmp;
    return ST_OK;
}

/* Menampilkan labirin ke layar, satu baris layar per baris grid. */
void maze_print(const Maze *m)
{
    int r;

    if (m == NULL || m->grid == NULL) {
        return;
    }
    for (r = 0; r < m->h; r++) {
        printf("%s\n", m->grid[r]);
    }
}

/* Memuat labirin dari berkas; pemeriksaan menyalin baris yang sudah valid. */
static Status load_rows(char **cur, int h, int w, char ***rows_out)
{
    char **rows = (char **)util_calloc((size_t)h, sizeof(char *));
    int filled = 0;
    int r;
    int c;
    Status st = ST_OK;

    if (rows == NULL) {
        return ST_NO_MEMORY;
    }
    for (r = 0; r < h && st == ST_OK; r++) {
        char *line = take_line(cur);
        if (line == NULL || (int)strlen(line) != w) {
            st = ST_BAD_FORMAT;
            break;
        }
        for (c = 0; c < w; c++) {
            if (!valid_cell_char(line[c])) {
                st = ST_BAD_FORMAT;
                break;
            }
        }
        if (st != ST_OK) {
            break;
        }
        rows[r] = (char *)util_calloc((size_t)w + 1, sizeof(char));
        if (rows[r] == NULL) {
            st = ST_NO_MEMORY;
            break;
        }
        memcpy(rows[r], line, (size_t)w);
        rows[r][w] = '\0';
        filled++;
    }
    if (st != ST_OK) {
        for (c = 0; c < filled; c++) {
            util_free(rows[c]);
        }
        util_free(rows);
        return st;
    }
    *rows_out = rows;
    return ST_OK;
}

/* Melepas array baris hasil pembacaan berkas. */
static void free_rows(char **rows, int h)
{
    int r;

    for (r = 0; r < h; r++) {
        util_free(rows[r]);
    }
    util_free(rows);
}

/* Membaca header dimensi dan memeriksa rentangnya. */
static Status read_header(char **cur, int *h, int *w)
{
    char *header = take_line(cur);

    if (header == NULL || parse_two_ints(header, h, w) != ST_OK) {
        return ST_BAD_FORMAT;
    }
    if (*h < 5 || *h > 61 || *w < 5 || *w > 61) {
        return ST_BAD_FORMAT;
    }
    return ST_OK;
}

/* Menyatakan sisa isi berkas hanya berisi baris kosong. */
static int only_blank_lines(char *cur)
{
    for (;;) {
        char *rest = take_line(&cur);
        if (rest == NULL) {
            return 1;
        }
        if (rest[0] != '\0') {
            return 0;
        }
    }
}

/* Membangun labirin dari baris yang sudah valid dan melepas baris tersebut. */
static Status build_maze(char **rows, int h, int w, Maze *out)
{
    int n_s = 0;
    int n_e = 0;
    int r;
    int c;
    Maze tmp;
    Status st;

    count_markers((const char **)rows, h, w, &n_s, &n_e);
    if (n_s != 1 || n_e != 1 || !border_ok((const char **)rows, h, w)) {
        free_rows(rows, h);
        return ST_BAD_FORMAT;
    }
    memset(&tmp, 0, sizeof tmp);
    st = maze_create(&tmp, h, w);
    if (st != ST_OK) {
        free_rows(rows, h);
        return st;
    }
    for (r = 0; r < h; r++) {
        for (c = 0; c < w; c++) {
            char ch = rows[r][c];
            tmp.grid[r][c] = (ch == '*') ? '.' : ch;
            if (ch == 'S') {
                tmp.start.r = r;
                tmp.start.c = c;
            } else if (ch == 'E') {
                tmp.end.r = r;
                tmp.end.c = c;
            }
        }
    }
    free_rows(rows, h);
    *out = tmp;
    return ST_OK;
}

/* Memuat labirin dari berkas teks dan menggantinya hanya bila seluruhnya valid. */
Status maze_load(const char *path, Maze *out)
{
    char *data = NULL;
    size_t len = 0;
    char *cur;
    char **rows = NULL;
    int h = 0;
    int w = 0;
    Maze tmp;
    Status st;

    if (path == NULL || out == NULL) {
        return ST_BAD_INPUT;
    }
    st = util_read_file(path, &data, &len);
    if (st != ST_OK) {
        return st;
    }
    cur = data;
    st = read_header(&cur, &h, &w);
    if (st == ST_OK) {
        st = load_rows(&cur, h, w, &rows);
    }
    if (st == ST_OK && !only_blank_lines(cur)) {
        free_rows(rows, h);
        st = ST_BAD_FORMAT;
    }
    util_free(data);
    if (st != ST_OK) {
        return st;
    }
    st = build_maze(rows, h, w, &tmp);
    if (st != ST_OK) {
        return st;
    }
    maze_destroy(out);
    *out = tmp;
    return ST_OK;
}

/* Menyusun teks berkas labirin dan menulisnya ke disk. */
Status maze_save(const char *path, const Maze *m)
{
    char *text;
    size_t cap;
    size_t used = 0;
    int r;
    Status st;

    if (path == NULL || m == NULL || m->grid == NULL) {
        return ST_BAD_INPUT;
    }
    cap = ((size_t)m->w + 1) * ((size_t)m->h + 1) + 32;
    text = (char *)util_malloc(cap);
    if (text == NULL) {
        return ST_NO_MEMORY;
    }
    used += (size_t)snprintf(text + used, cap - used, "%d %d\n", m->h, m->w);
    for (r = 0; r < m->h; r++) {
        used += (size_t)snprintf(text + used, cap - used, "%s\n", m->grid[r]);
    }
    st = util_write_file(path, text, used);
    util_free(text);
    return st;
}

/* Menyatakan koordinat berada di dalam grid. */
int maze_in_bounds(const Maze *m, int r, int c)
{
    if (m == NULL) {
        return 0;
    }
    return r >= 0 && r < m->h && c >= 0 && c < m->w;
}
