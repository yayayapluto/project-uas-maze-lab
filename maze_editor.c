#include "maze_editor.h"

#include "maze_solve.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define EDITOR_MAX_ARGS 4
#define EDITOR_MIN_DIM 5
#define EDITOR_MAX_DIM 61

/* Menulis pesan hasil perintah ke buffer keluaran dengan batas panjang. */
static void say(char *out, size_t out_size, const char *text)
{
    if (out == NULL || out_size == 0) {
        return;
    }
    snprintf(out, out_size, "%s", text);
}

/* Mengurai bilangan bulat penuh; mengembalikan 0 bila bukan angka utuh. */
static int parse_int_full(const char *s, int *out)
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

/* Memisahkan baris perintah menjadi token dan mengembalikan jumlah token. */
static int split_tokens(char *line, char *tokens[], int max)
{
    int count = 0;
    char *tok = strtok(line, " \t\r\n");

    while (tok != NULL && count < max) {
        tokens[count++] = tok;
        tok = strtok(NULL, " \t\r\n");
    }
    if (tok != NULL) {
        count++;
    }
    return count;
}

/* Menyatakan baris perintah berarti keluar dari editor. */
static int is_quit(const char *line)
{
    char buf[UTIL_MAX_LINE];
    char *tokens[2];
    int n;

    if (line == NULL) {
        return 0;
    }
    snprintf(buf, sizeof buf, "%s", line);
    n = split_tokens(buf, tokens, 2);
    if (n < 1) {
        return 0;
    }
    util_to_lower(tokens[0]);
    return strcmp(tokens[0], "quit") == 0;
}

/* Menambahkan satu entri ke stack undo; melepasnya bila alokasi gagal. */
static void undo_push(MazeUndoStack *st, int kind, int r, int c, char prev)
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

/* Mengosongkan stack undo tanpa melepas buffernya. */
static void undo_clear(MazeUndoStack *st)
{
    st->count = 0;
}

/* Melepas isi stack undo. */
void maze_undo_stack_free(MazeUndoStack *st)
{
    if (st == NULL) {
        return;
    }
    util_free(st->items);
    st->items = NULL;
    st->count = 0;
    st->capacity = 0;
}

/* Memeriksa koordinat berada di dalam labirin dan bukan sel tepi. */
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

/* Membuat labirin kosong dengan tepi dinding, bagian dalam jalan, serta penanda S dan E. */
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

/* Menjalankan perintah new h w dan menggantinya hanya bila berhasil. */
static Status cmd_new(Maze *slot, MazeUndoStack *st, int h, int w, char *out, size_t out_size)
{
    Maze tmp;
    Status status;

    if (h < EDITOR_MIN_DIM || h > EDITOR_MAX_DIM || w < EDITOR_MIN_DIM || w > EDITOR_MAX_DIM) {
        say(out, out_size, "Ukuran harus antara 5 dan 61. Gunakan help untuk daftar perintah.");
        return ST_OUT_OF_RANGE;
    }
    memset(&tmp, 0, sizeof tmp);
    status = make_blank(&tmp, h, w);
    if (status != ST_OK) {
        say(out, out_size, util_status_message(status));
        return status;
    }
    maze_destroy(slot);
    *slot = tmp;
    undo_clear(st);
    say(out, out_size, "Labirin baru dibuat.");
    return ST_OK;
}

/* Menjalankan perintah wall dan open pada satu sel. */
static Status cmd_cell(Maze *m, MazeUndoStack *st, int r, int c, int kind, char *out, size_t out_size)
{
    char target = (kind == UNDO_WALL) ? '#' : '.';
    char prev;

    if (!coord_ok(m, r, c)) {
        say(out, out_size, "Ditolak: sel berada di luar labirin atau di tepi.");
        return ST_OUT_OF_RANGE;
    }
    if (m->grid[r][c] == 'S' || m->grid[r][c] == 'E') {
        say(out, out_size, "Ditolak: sel berisi titik awal atau titik tujuan.");
        return ST_OUT_OF_RANGE;
    }
    prev = m->grid[r][c];
    if (prev == target) {
        say(out, out_size, "Sel sudah bernilai tersebut, tidak ada yang diubah.");
        return ST_OK;
    }
    m->grid[r][c] = target;
    undo_push(st, kind, r, c, prev);
    say(out, out_size, (kind == UNDO_WALL) ? "Dinding dibuat." : "Sel dibuka.");
    return ST_OK;
}

/* Menjalankan perintah start dan end yang memindahkan penanda. */
static Status cmd_marker(Maze *m, MazeUndoStack *st, int r, int c, int kind, char *out, size_t out_size)
{
    char marker = (kind == UNDO_START) ? 'S' : 'E';
    char other = (kind == UNDO_START) ? 'E' : 'S';
    MazeCell old;
    char prev;

    if (!coord_ok(m, r, c)) {
        say(out, out_size, "Ditolak: sel berada di luar labirin atau di tepi.");
        return ST_OUT_OF_RANGE;
    }
    if (m->grid[r][c] == other) {
        say(out, out_size, "Ditolak: sel sudah berisi titik awal atau titik tujuan lain.");
        return ST_OUT_OF_RANGE;
    }
    old = (kind == UNDO_START) ? m->start : m->end;
    if (old.r == r && old.c == c) {
        say(out, out_size, "Titik tersebut sudah berada di posisi itu.");
        return ST_OUT_OF_RANGE;
    }
    prev = m->grid[r][c];
    m->grid[old.r][old.c] = '.';
    m->grid[r][c] = marker;
    undo_push(st, kind, old.r, old.c, prev);
    if (kind == UNDO_START) {
        m->start.r = r;
        m->start.c = c;
    } else {
        m->end.r = r;
        m->end.c = c;
    }
    say(out, out_size, "Titik dipindahkan.");
    return ST_OK;
}

/* Membatalkan perintah ubah terakhir memakai stack undo. */
static Status cmd_undo(Maze *m, MazeUndoStack *st, char *out, size_t out_size)
{
    MazeUndoEntry e;

    if (st->count == 0) {
        say(out, out_size, "Tidak ada perubahan yang dapat dibatalkan.");
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
    say(out, out_size, "Perubahan terakhir dibatalkan.");
    return ST_OK;
}

/* Menampilkan labirin dengan penomoran baris dan kolom. */
static void cmd_show(const Maze *m)
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

/* Menjalankan perintah check: validasi lalu pencarian jalur terpendek. */
static void cmd_check(const Maze *m, char *out, size_t out_size)
{
    char msg[128];
    int isolated = 0;
    MazeSolveResult res;
    MazeCell *path = NULL;
    char text[192];
    size_t used;
    Status st = maze_validate(m, &isolated, msg, sizeof msg);

    if (st != ST_OK) {
        say(out, out_size, msg);
        return;
    }
    if (maze_solve_bfs(m, &res, &path) != ST_OK) {
        say(out, out_size, "Pencarian jalur gagal.");
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
    say(out, out_size, text);
}

/* Menyusun path berkas editor dari nama yang diketik pengguna. */
static void build_path(char *buf, size_t size, const char *name)
{
    size_t len = strlen(name);

    if (len > 4 && strcmp(name + len - 4, ".txt") == 0) {
        snprintf(buf, size, "data/%s", name);
    } else {
        snprintf(buf, size, "data/%s.txt", name);
    }
}

/* Menjalankan perintah save nama. */
static Status cmd_save(const Maze *m, const char *name, char *out, size_t out_size)
{
    char path[UTIL_MAX_PATH];
    Status st;

    build_path(path, sizeof path, name);
    st = maze_save(path, m);
    say(out, out_size, (st == ST_OK) ? "Labirin disimpan." : util_status_message(st));
    return st;
}

/* Menjalankan perintah load nama tanpa merusak labirin aktif bila gagal. */
static Status cmd_load(Maze *slot, MazeUndoStack *st, const char *name, char *out, size_t out_size)
{
    char path[UTIL_MAX_PATH];
    Maze tmp;
    Status status;

    build_path(path, sizeof path, name);
    memset(&tmp, 0, sizeof tmp);
    status = maze_load(path, &tmp);
    if (status != ST_OK) {
        say(out, out_size, util_status_message(status));
        return status;
    }
    maze_destroy(slot);
    *slot = tmp;
    undo_clear(st);
    say(out, out_size, "Labirin dimuat.");
    return ST_OK;
}

/* Menjalankan perintah help. */
static void cmd_help(char *out, size_t out_size)
{
    say(out, out_size,
        "Perintah: new h w | wall r c | open r c | start r c | end r c | show | check | "
        "undo | save nama | load nama | help | quit");
}

/* Menjalankan perintah yang tidak memerlukan labirin aktif. */
static Status exec_without_maze(Maze *slot, MazeUndoStack *st, char **tokens, int count,
                                 char *out, size_t out_size)
{
    if (strcmp(tokens[0], "new") == 0) {
        int h;
        int w;
        if (count != 3 || !parse_int_full(tokens[1], &h) || !parse_int_full(tokens[2], &w)) {
            say(out, out_size, "Argumen tidak valid. Gunakan help untuk daftar perintah.");
            return ST_BAD_INPUT;
        }
        return cmd_new(slot, st, h, w, out, out_size);
    }
    if (strcmp(tokens[0], "load") == 0) {
        if (count != 2) {
            say(out, out_size, "Argumen tidak valid. Gunakan help untuk daftar perintah.");
            return ST_BAD_INPUT;
        }
        return cmd_load(slot, st, tokens[1], out, out_size);
    }
    if (strcmp(tokens[0], "help") == 0) {
        cmd_help(out, out_size);
        return ST_OK;
    }
    say(out, out_size, "Keluar dari editor.");
    return ST_OK;
}

/* Menjalankan perintah yang mengubah isi labirin aktif. */
static Status exec_with_maze(Maze *slot, MazeUndoStack *st, char **tokens, int count,
                             char *out, size_t out_size)
{
    int r;
    int c;

    if (strcmp(tokens[0], "wall") == 0 || strcmp(tokens[0], "open") == 0 ||
        strcmp(tokens[0], "start") == 0 || strcmp(tokens[0], "end") == 0) {
        int kind;
        if (count != 3 || !parse_int_full(tokens[1], &r) || !parse_int_full(tokens[2], &c)) {
            say(out, out_size, "Argumen tidak valid. Gunakan help untuk daftar perintah.");
            return ST_BAD_INPUT;
        }
        if (strcmp(tokens[0], "wall") == 0) {
            kind = UNDO_WALL;
        } else if (strcmp(tokens[0], "open") == 0) {
            kind = UNDO_OPEN;
        } else if (strcmp(tokens[0], "start") == 0) {
            kind = UNDO_START;
        } else {
            kind = UNDO_END;
        }
        if (kind == UNDO_WALL || kind == UNDO_OPEN) {
            return cmd_cell(slot, st, r, c, kind, out, out_size);
        }
        return cmd_marker(slot, st, r, c, kind, out, out_size);
    }
    if (count != 1) {
        say(out, out_size, "Argumen tidak valid. Gunakan help untuk daftar perintah.");
        return ST_BAD_INPUT;
    }
    if (strcmp(tokens[0], "show") == 0) {
        cmd_show(slot);
        say(out, out_size, "Labirin ditampilkan.");
        return ST_OK;
    }
    if (strcmp(tokens[0], "check") == 0) {
        cmd_check(slot, out, out_size);
        return ST_OK;
    }
    if (strcmp(tokens[0], "undo") == 0) {
        return cmd_undo(slot, st, out, out_size);
    }
    if (strcmp(tokens[0], "help") == 0) {
        cmd_help(out, out_size);
        return ST_OK;
    }
    say(out, out_size, "Keluar dari editor.");
    return ST_OK;
}

/* Menjalankan satu baris perintah editor dan menulis pesan hasilnya. */
Status maze_editor_exec(Maze *slot, MazeUndoStack *st, const char *line, char *out, size_t out_size)
{
    char buf[UTIL_MAX_LINE];
    char *tokens[EDITOR_MAX_ARGS];
    int count;

    if (slot == NULL || st == NULL || line == NULL) {
        return ST_BAD_INPUT;
    }
    snprintf(buf, sizeof buf, "%s", line);
    count = split_tokens(buf, tokens, EDITOR_MAX_ARGS);
    if (count == 0) {
        say(out, out_size, "Perintah kosong. Gunakan help untuk daftar perintah.");
        return ST_BAD_INPUT;
    }
    util_to_lower(tokens[0]);
    if (slot->grid == NULL) {
        if (strcmp(tokens[0], "wall") == 0 || strcmp(tokens[0], "open") == 0 ||
            strcmp(tokens[0], "start") == 0 || strcmp(tokens[0], "end") == 0 ||
            strcmp(tokens[0], "show") == 0 || strcmp(tokens[0], "check") == 0 ||
            strcmp(tokens[0], "undo") == 0 || strcmp(tokens[0], "save") == 0) {
            say(out, out_size, "Belum ada labirin aktif. Gunakan new atau load.");
            return ST_BAD_INPUT;
        }
        return exec_without_maze(slot, st, tokens, count, out, out_size);
    }
    if (strcmp(tokens[0], "new") == 0 || strcmp(tokens[0], "load") == 0 ||
        strcmp(tokens[0], "save") == 0) {
        if (strcmp(tokens[0], "new") == 0) {
            return exec_without_maze(slot, st, tokens, count, out, out_size);
        }
        if (count != 2) {
            say(out, out_size, "Argumen tidak valid. Gunakan help untuk daftar perintah.");
            return ST_BAD_INPUT;
        }
        if (strcmp(tokens[0], "save") == 0) {
            return cmd_save(slot, tokens[1], out, out_size);
        }
        return cmd_load(slot, st, tokens[1], out, out_size);
    }
    return exec_with_maze(slot, st, tokens, count, out, out_size);
}

/* Menjalankan sesi editor sampai perintah quit diterima. */
Status maze_run_editor(Maze *slot)
{
    MazeUndoStack st;
    char line[UTIL_MAX_LINE];
    char out[UTIL_MAX_LINE];

    if (slot == NULL) {
        return ST_BAD_INPUT;
    }
    memset(&st, 0, sizeof st);
    say(out, sizeof out, "Editor labirin. Ketik help untuk daftar perintah, quit untuk keluar.");
    printf("%s\n", out);
    for (;;) {
        Status s = util_read_line("> ", line, sizeof line);
        if (s != ST_OK) {
            break;
        }
        if (is_quit(line)) {
            break;
        }
        maze_editor_exec(slot, &st, line, out, sizeof out);
        printf("%s\n", out);
    }
    maze_undo_stack_free(&st);
    return ST_OK;
}