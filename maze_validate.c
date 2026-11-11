#include "maze_core.h"

#include <stdio.h>
#include <string.h>

/* Menyatakan sel merupakan jalan yang boleh dilalui. */
static int walkable(char ch)
{
    return ch == '.' || ch == 'S' || ch == 'E' || ch == '*';
}

/* Flood fill rekursif dari (r, c) menandai sel terjangkau; mengembalikan jumlah sel baru. */
static int flood(const Maze *m, int r, int c, char *seen)
{
    static const int dr[4] = { -1, 0, 1, 0 };
    static const int dc[4] = { 0, 1, 0, -1 };
    int count = 0;
    int d;

    if (!maze_in_bounds(m, r, c) || seen[r * m->w + c]) {
        return 0;
    }
    if (!walkable(m->grid[r][c])) {
        return 0;
    }
    seen[r * m->w + c] = 1;
    count = 1;
    for (d = 0; d < 4; d++) {
        count += flood(m, r + dr[d], c + dc[d], seen);
    }
    return count;
}

/* Menghitung jumlah seluruh sel jalan dalam labirin. */
static int count_walkable(const Maze *m)
{
    int total = 0;
    int r;
    int c;

    for (r = 0; r < m->h; r++) {
        for (c = 0; c < m->w; c++) {
            if (walkable(m->grid[r][c])) {
                total++;
            }
        }
    }
    return total;
}

/* Memeriksa seluruh sel tepi adalah dinding. */
static int borders_are_walls(const Maze *m)
{
    int r;
    int c;

    for (c = 0; c < m->w; c++) {
        if (m->grid[0][c] != '#' || m->grid[m->h - 1][c] != '#') {
            return 0;
        }
    }
    for (r = 0; r < m->h; r++) {
        if (m->grid[r][0] != '#' || m->grid[r][m->w - 1] != '#') {
            return 0;
        }
    }
    return 1;
}

/* Menghitung jumlah titik awal S dan titik tujuan E. */
static void count_markers(const Maze *m, int *n_s, int *n_e)
{
    int r;
    int c;

    *n_s = 0;
    *n_e = 0;
    for (r = 0; r < m->h; r++) {
        for (c = 0; c < m->w; c++) {
            if (m->grid[r][c] == 'S') {
                (*n_s)++;
            } else if (m->grid[r][c] == 'E') {
                (*n_e)++;
            }
        }
    }
}

/* Menjalankan flood fill dari S dan melaporkan keterjangkauan E. */
static Status reachability(const Maze *m, int *reachable, int *e_reachable)
{
    char *seen = (char *)util_calloc((size_t)(m->h * m->w), sizeof(char));

    if (seen == NULL) {
        return ST_NO_MEMORY;
    }
    *reachable = flood(m, m->start.r, m->start.c, seen);
    *e_reachable = seen[m->end.r * m->w + m->end.c];
    util_free(seen);
    return ST_OK;
}

/* Memeriksa bentuk labirin: tepi dinding, tepat satu S, dan tepat satu E. */
static Status check_shape(const Maze *m, char *msg, size_t msg_size)
{
    int n_s = 0;
    int n_e = 0;

    count_markers(m, &n_s, &n_e);
    if (!borders_are_walls(m)) {
        snprintf(msg, msg_size, "Tepi labirin bukan seluruhnya dinding.");
        return ST_BAD_FORMAT;
    }
    if (n_s != 1) {
        snprintf(msg, msg_size, "Harus ada tepat satu titik awal S, ditemukan %d.", n_s);
        return ST_BAD_FORMAT;
    }
    if (n_e != 1) {
        snprintf(msg, msg_size, "Harus ada tepat satu titik tujuan E, ditemukan %d.", n_e);
        return ST_BAD_FORMAT;
    }
    return ST_OK;
}

/* Memeriksa tepi, jumlah S dan E, keterjangkauan E, dan melaporkan sel terisolasi. */
Status maze_validate(const Maze *m, int *isolated, char *msg, size_t msg_size)
{
    int reachable = 0;
    int e_reachable = 0;
    int total;
    Status st;

    if (m == NULL || m->grid == NULL) {
        if (msg != NULL && msg_size > 0) {
            snprintf(msg, msg_size, "Labirin belum ada.");
        }
        return ST_BAD_FORMAT;
    }
    if (msg != NULL && msg_size > 0) {
        msg[0] = '\0';
    }
    st = check_shape(m, msg, msg_size);
    if (st != ST_OK) {
        return st;
    }
    st = reachability(m, &reachable, &e_reachable);
    if (st != ST_OK) {
        return st;
    }
    total = count_walkable(m);
    if (isolated != NULL) {
        *isolated = total - reachable;
    }
    if (!e_reachable) {
        snprintf(msg, msg_size, "Titik tujuan E tidak terjangkau dari S.");
        return ST_BAD_FORMAT;
    }
    if (total - reachable > 0) {
        snprintf(msg, msg_size, "Labirin valid, %d sel jalan terisolasi.", total - reachable);
    } else {
        snprintf(msg, msg_size, "Labirin valid tanpa sel terisolasi.");
    }
    return ST_OK;
}

/* Menghitung sel jalan dengan tepat satu tetangga jalan; S dan E bukan kandidat. */
int maze_count_dead_ends(const Maze *m)
{
    static const int dr[4] = { -1, 0, 1, 0 };
    static const int dc[4] = { 0, 1, 0, -1 };
    int dead = 0;
    int r;
    int c;
    int d;

    if (m == NULL || m->grid == NULL) {
        return 0;
    }
    for (r = 0; r < m->h; r++) {
        for (c = 0; c < m->w; c++) {
            int neighbours = 0;
            if (!walkable(m->grid[r][c]) || m->grid[r][c] == 'S' || m->grid[r][c] == 'E') {
                continue;
            }
            for (d = 0; d < 4; d++) {
                int nr = r + dr[d];
                int nc = c + dc[d];
                if (maze_in_bounds(m, nr, nc) && walkable(m->grid[nr][nc])) {
                    neighbours++;
                }
            }
            if (neighbours == 1) {
                dead++;
            }
        }
    }
    return dead;
}
