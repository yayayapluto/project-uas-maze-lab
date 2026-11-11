#include "maze_editor_cmd.h"

#include "../maze_solve/maze_solve.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define EDITOR_MIN_DIM 5
#define EDITOR_MAX_DIM 61

/* Tulis pesan hasil perintah ke buffer keluaran, dengan batas panjang. */
void maze_editor_say(char *out, size_t out_size, const char *text)
{
    if (out == NULL || out_size == 0) {
        return;
    }
    snprintf(out, out_size, "%s", text);
}

/* Parse bilangan bulat penuh; kembalikan 0 kalau bukan angka utuh. */
int maze_editor_parse_int(const char *s, int *out)
{
    char *end = NULL;
    long value;

    if (s == NULL || s[0] == '\0') {
        return 0;
    }
    value = strtol(s, &end, 10);
    if (end == s || *end != '\0') {
        return 0;
    }
    if (value < -2147483647L || value > 2147483647L) {
        return 0;
    }
    *out = (int)value;
    return 1;
}

/* Push satu entri ke stack undo; kalau alokasi gagal, entri dilewati. */
void maze_editor_undo_push(MazeUndoStack *st, int kind, int r, int c, char prev)
{
    if (st->count == st->capacity) {
        int newcap = (st->capacity == 0) ? 8 : st->capacity * 2;
        MazeUndoEntry *tmp = (MazeUndoEntry *)util_realloc(st->items, (size_t)newcap * sizeof(MazeUndoEntry));
        if (tmp == NULL) {
            return;
        }
        st->items = tmp;
        st->capacity = newcap;
    }
    st->items[st->count].kind = kind;
    st->items[st->count].r = r;
    st->items[st->count].c = c;
    st->items[st->count].prev = prev;
    st->count++;
}

/* Pastikan koordinat ada di dalam labirin dan bukan sel tepi. */
static int coord_ok(const Maze *m, int r, int c)
{
    if (!maze_in_bounds(m, r, c)) {
        return 0;
    }
    if (r == 0 || c == 0 || r == m->h - 1 || c == m->w - 1) {
        return 0;
    }
    return 1;
}

/* Buat labirin kosong: tepi dinding, bagian dalam jalan, plus penanda S dan E. */
static Status make_blank(Maze *tmp, int h, int w)
{
    int r;
    int c;
    Status st = maze_create(tmp, h, w);

    if (st != ST_OK) {
        return st;
    }
    for (r = 1; r < h - 1; r++) {
        for (c = 1; c < w - 1; c++) {
            tmp->grid[r][c] = '.';
        }
    }
    tmp->grid[1][1] = 'S';
    tmp->grid[h - 2][w - 2] = 'E';
    tmp->start.r = 1;
    tmp->start.c = 1;
    tmp->end.r = h - 2;
    tmp->end.c = w - 2;
    return ST_OK;
}

/* Jalankan perintah new h w; struct lama diganti hanya kalau berhasil. */
Status maze_editor_cmd_new(Maze *slot, MazeUndoStack *st, int h, int w, char *out, size_t out_size)
{
    Maze tmp;
    Status status;

    if (h < EDITOR_MIN_DIM || h > EDITOR_MAX_DIM || w < EDITOR_MIN_DIM || w > EDITOR_MAX_DIM) {
        maze_editor_say(out, out_size, "Ukuran harus antara 5 dan 61. Gunakan help untuk daftar perintah.");
        return ST_OUT_OF_RANGE;
    }
    memset(&tmp, 0, sizeof tmp);
    status = make_blank(&tmp, h, w);
    if (status != ST_OK) {
        maze_editor_say(out, out_size, util_status_message(status));
        return status;
    }
    maze_destroy(slot);
    *slot = tmp;
    st->count = 0;
    maze_editor_say(out, out_size, "Labirin baru dibuat.");
    return ST_OK;
}

/* Jalankan perintah wall dan open pada satu sel. */
Status maze_editor_cmd_cell(Maze *m, MazeUndoStack *st, int r, int c, int kind,
                            char *out, size_t out_size)
{
    char target = (kind == UNDO_WALL) ? '#' : '.';
    char prev;

    if (!coord_ok(m, r, c)) {
        maze_editor_say(out, out_size, "Ditolak: sel berada di luar labirin atau di tepi.");
        return ST_OUT_OF_RANGE;
    }
    if (m->grid[r][c] == 'S' || m->grid[r][c] == 'E') {
        maze_editor_say(out, out_size, "Ditolak: sel berisi titik awal atau titik tujuan.");
        return ST_OUT_OF_RANGE;
    }
    prev = m->grid[r][c];
    if (prev == target) {
        maze_editor_say(out, out_size, "Sel sudah bernilai tersebut, tidak ada yang diubah.");
        return ST_OK;
    }
    m->grid[r][c] = target;
    maze_editor_undo_push(st, kind, r, c, prev);
    maze_editor_say(out, out_size, (kind == UNDO_WALL) ? "Dinding dibuat." : "Sel dibuka.");
    return ST_OK;
}

/* Jalankan perintah start dan end untuk memindahkan penanda. */
Status maze_editor_cmd_marker(Maze *m, MazeUndoStack *st, int r, int c, int kind,
                               char *out, size_t out_size)
{
    char marker = (kind == UNDO_START) ? 'S' : 'E';
    char other = (kind == UNDO_START) ? 'E' : 'S';
    MazeCell old;
    char prev;

    if (!coord_ok(m, r, c)) {
        maze_editor_say(out, out_size, "Ditolak: sel berada di luar labirin atau di tepi.");
        return ST_OUT_OF_RANGE;
    }
    if (m->grid[r][c] == other) {
        maze_editor_say(out, out_size, "Ditolak: sel sudah berisi titik awal atau titik tujuan lain.");
        return ST_OUT_OF_RANGE;
    }
    old = (kind == UNDO_START) ? m->start : m->end;
    if (old.r == r && old.c == c) {
        maze_editor_say(out, out_size, "Titik tersebut sudah berada di posisi itu.");
        return ST_OUT_OF_RANGE;
    }
    prev = m->grid[r][c];
    m->grid[old.r][old.c] = '.';
    m->grid[r][c] = marker;
    maze_editor_undo_push(st, kind, old.r, old.c, prev);
    if (kind == UNDO_START) {
        m->start.r = r;
        m->start.c = c;
    } else {
        m->end.r = r;
        m->end.c = c;
    }
    maze_editor_say(out, out_size, "Titik dipindahkan.");
    return ST_OK;
}

/* Batalkan perubahan terakhir memakai stack undo. */
Status maze_editor_cmd_undo(Maze *m, MazeUndoStack *st, char *out, size_t out_size)
{
    MazeUndoEntry e;

    if (st->count == 0) {
        maze_editor_say(out, out_size, "Tidak ada perubahan yang dapat dibatalkan.");
        return ST_OK;
    }
    e = st->items[st->count - 1];
    st->count--;
    if (e.kind == UNDO_START || e.kind == UNDO_END) {
        MazeCell *cur = (e.kind == UNDO_START) ? &m->start : &m->end;
        char marker = (e.kind == UNDO_START) ? 'S' : 'E';
        m->grid[cur->r][cur->c] = e.prev;
        m->grid[e.r][e.c] = marker;
        cur->r = e.r;
        cur->c = e.c;
    } else {
        m->grid[e.r][e.c] = e.prev;
    }
    maze_editor_say(out, out_size, "Perubahan terakhir dibatalkan.");
    return ST_OK;
}

/* Cetak labirin dengan nomor baris dan kolom. */
void maze_editor_cmd_show(const Maze *m)
{
    int c;
    int r;

    printf("    ");
    for (c = 0; c < m->w; c++) {
        printf("%d", (c / 10) % 10);
    }
    printf("\n    ");
    for (c = 0; c < m->w; c++) {
        printf("%d", c % 10);
    }
    printf("\n");
    for (r = 0; r < m->h; r++) {
        printf("%3d %s\n", r, m->grid[r]);
    }
}

/* Jalankan perintah check: validasi dulu, lalu cari jalur terpendek. */
void maze_editor_cmd_check(const Maze *m, char *out, size_t out_size)
{
    char msg[128];
    int isolated = 0;
    MazeSolveResult res;
    MazeCell *path = NULL;
    char text[192];
    size_t used;
    Status st = maze_validate(m, &isolated, msg, sizeof msg);

    if (st != ST_OK) {
        maze_editor_say(out, out_size, msg);
        return;
    }
    if (maze_solve_bfs(m, &res, &path) != ST_OK) {
        maze_editor_say(out, out_size, "Pencarian jalur gagal.");
        return;
    }
    if (res.found) {
        snprintf(text, sizeof text, "Labirin dapat diselesaikan. Panjang jalur terpendek: %d langkah.", res.length);
    } else {
        snprintf(text, sizeof text, "Labirin tidak dapat diselesaikan.");
    }
    used = strlen(text);
    if (isolated > 0) {
        snprintf(text + used, sizeof text - used, " %d sel jalan terisolasi.", isolated);
    }
    util_free(path);
    maze_editor_say(out, out_size, text);
}

/* Susun path file editor dari nama yang diketik pengguna. */
static void build_path(char *buf, size_t size, const char *name)
{
    size_t len = strlen(name);

    if (len > 4 && strcmp(name + len - 4, ".txt") == 0) {
        snprintf(buf, size, "data/%s", name);
    } else {
        snprintf(buf, size, "data/%s.txt", name);
    }
}

/* Jalankan perintah save nama. */
Status maze_editor_cmd_save(const Maze *m, const char *name, char *out, size_t out_size)
{
    char path[UTIL_MAX_PATH];
    Status st;

    build_path(path, sizeof path, name);
    st = maze_save(path, m);
    maze_editor_say(out, out_size, (st == ST_OK) ? "Labirin disimpan." : util_status_message(st));
    return st;
}

/* Jalankan perintah load nama tanpa merusak labirin aktif kalau gagal. */
Status maze_editor_cmd_load(Maze *slot, MazeUndoStack *st, const char *name,
                             char *out, size_t out_size)
{
    char path[UTIL_MAX_PATH];
    Maze tmp;
    Status status;

    build_path(path, sizeof path, name);
    memset(&tmp, 0, sizeof tmp);
    status = maze_load(path, &tmp);
    if (status != ST_OK) {
        maze_editor_say(out, out_size, util_status_message(status));
        return status;
    }
    maze_destroy(slot);
    *slot = tmp;
    st->count = 0;
    maze_editor_say(out, out_size, "Labirin dimuat.");
    return ST_OK;
}

/* Menjalankan perintah help. */
void maze_editor_cmd_help(char *out, size_t out_size)
{
    maze_editor_say(out, out_size,
        "Perintah: new h w | wall r c | open r c | start r c | end r c | show | check | "
        "undo | save nama | load nama | help | quit");
}
