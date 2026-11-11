#include "../maze_lab/maze_lab.h"

#include "../maze_editor/maze_editor.h"
#include "../maze_gen/maze_gen.h"
#include "../maze_solve/maze_solve.h"
#include "maze_test.h"

#include <stdio.h>
#include <string.h>

#define SMALL_MAZE "data/kecil.txt"
#define LOOP_MAZE "data/loop_putar.txt"
#define DEAD_MAZE "data/tanpa_jalan.txt"
#define ISOLATED_MAZE "data/terisolasi.txt"
#define TMP_MAZE "data/tmp_u01.txt"

/* Hitung jumlah sel jalan pada labirin. */
static int count_walkable(const Maze *m)
{
    int total = 0;
    int r;
    int c;

    for (r = 0; r < m->h; r++) {
        for (c = 0; c < m->w; c++) {
            if (m->grid[r][c] != '#') {
                total++;
            }
        }
    }
    return total;
}

/* Pastikan dua labirin punya grid, titik awal, dan titik tujuan yang sama. */
static int same_maze(const Maze *a, const Maze *b)
{
    int r;

    if (a->h != b->h || a->w != b->w) {
        return 0;
    }
    if (a->start.r != b->start.r || a->start.c != b->start.c) {
        return 0;
    }
    if (a->end.r != b->end.r || a->end.c != b->end.c) {
        return 0;
    }
    for (r = 0; r < a->h; r++) {
        if (strcmp(a->grid[r], b->grid[r]) != 0) {
            return 0;
        }
    }
    return 1;
}

/* Pastikan grid dua labirin sama persis. */
static int same_grid(const Maze *a, const Maze *b)
{
    int r;

    if (a->h != b->h || a->w != b->w) {
        return 0;
    }
    for (r = 0; r < a->h; r++) {
        if (strcmp(a->grid[r], b->grid[r]) != 0) {
            return 0;
        }
    }
    return 1;
}

/* U01: simpan lalu muat, hasilnya harus identik. */
static int test_save_load(void)
{
    Maze src;
    Maze dst;
    Status st;
    int ok = 1;

    util_test_begin("U01 Simpan lalu muat");
    memset(&src, 0, sizeof src);
    memset(&dst, 0, sizeof dst);
    st = maze_load(SMALL_MAZE, &src);
    util_check(st == ST_OK, "U01 file contoh termuat");
    st = maze_save(TMP_MAZE, &src);
    util_check(st == ST_OK, "U01 labirin tersimpan");
    st = maze_load(TMP_MAZE, &dst);
    util_check(st == ST_OK, "U01 labirin termuat kembali");
    util_check(same_maze(&src, &dst), "U01 grid, S, dan E identik");
    remove(TMP_MAZE);
    maze_destroy(&src);
    maze_destroy(&dst);
    ok = util_test_end();
    return ok;
}

/* U02: tujuh file rusak harus ditolak dengan ST_BAD_FORMAT. */
static int test_broken_files(void)
{
    static const char *files[7] = {
        "data/rusak_tanpa_s.txt", "data/rusak_dua_e.txt", "data/rusak_beda_panjang.txt",
        "data/rusak_karakter.txt", "data/rusak_tepi.txt", "data/rusak_dimensi.txt",
        "data/rusak_jumlah_baris.txt"
    };
    Maze m;
    int i;
    int ok;

    util_test_begin("U02 File rusak ditolak");
    for (i = 0; i < 7; i++) {
        memset(&m, 0, sizeof m);
        ok = (maze_load(files[i], &m) == ST_BAD_FORMAT);
        util_check(ok, files[i]);
        maze_destroy(&m);
    }
    return util_test_end();
}

/* U03: seed yang sama harus menghasilkan grid identik pada kedua algoritma. */
static int test_same_seed(void)
{
    Maze a;
    Maze b;

    util_test_begin("U03 Generator dengan seed sama");
    memset(&a, 0, sizeof a);
    memset(&b, 0, sizeof b);
    maze_gen_backtracker(&a, 11, 11, 1234u, 0);
    maze_gen_backtracker(&b, 11, 11, 1234u, 0);
    util_check(same_grid(&a, &b), "U03 backtracker dua kali sama");
    maze_gen_prim(&a, 11, 11, 4321u, 0);
    maze_gen_prim(&b, 11, 11, 4321u, 0);
    util_check(same_grid(&a, &b), "U03 prim dua kali sama");
    maze_destroy(&a);
    maze_destroy(&b);
    return util_test_end();
}

/* U04: labirin sempurna punya 2L-1 sel jalan dan semuanya terhubung. */
static int test_perfect_maze(void)
{
    static const int sizes[3][2] = { { 5, 5 }, { 7, 9 }, { 11, 11 } };
    int i;
    int g;
    int ok;

    util_test_begin("U04 Labirin sempurna");
    for (g = 0; g < 2; g++) {
        for (i = 0; i < 3; i++) {
            Maze m;
            int h = sizes[i][0];
            int w = sizes[i][1];
            int logical = ((h - 1) / 2) * ((w - 1) / 2);
            int isolated = -1;
            char msg[128];
            memset(&m, 0, sizeof m);
            ok = (g == 0) ? maze_gen_backtracker(&m, h, w, 77u, 0)
                          : maze_gen_prim(&m, h, w, 77u, 0);
            util_check(ok == ST_OK, "U04 labirin sempurna terbentuk");
            util_check(count_walkable(&m) == 2 * logical - 1, "U04 jumlah sel jalan 2L-1");
            util_check(maze_validate(&m, &isolated, msg, sizeof msg) == ST_OK,
                       "U04 labirin valid dan semua sel terhubung");
            util_check(m.start.r == 1 && m.start.c == 1, "U04 titik awal di (1,1)");
            util_check(m.end.r == h - 2 && m.end.c == w - 2, "U04 titik tujuan di pojok dalam");
            maze_destroy(&m);
        }
    }
    return util_test_end();
}

/* U05 dan U06: BFS harus menemukan jalur terpendek dan tidak lebih panjang dari DFS. */
static int test_bfs_length(void)
{
    Maze m;
    MazeSolveResult bfs;
    MazeSolveResult dfs;
    MazeCell *path = NULL;
    int ok;

    util_test_begin("U05 dan U06 Panjang jalur BFS dan DFS");
    memset(&m, 0, sizeof m);
    maze_load(SMALL_MAZE, &m);
    maze_solve_bfs(&m, &bfs, &path);
    util_check(bfs.found == 1, "U05 BFS menemukan jalur pada labirin kecil");
    util_check(bfs.length == 7, "U05 panjang jalur BFS adalah 7 langkah");
    util_free(path);
    path = NULL;
    maze_solve_dfs(&m, &dfs, &path);
    util_check(dfs.found == 1, "U06 DFS menemukan jalur");
    util_check(dfs.length >= bfs.length, "U06 panjang DFS tidak lebih kecil dari BFS");
    util_free(path);
    maze_destroy(&m);
    ok = util_test_end();
    return ok;
}

/* U07: ketiga solver harus gagal pada labirin tanpa jalur. */
static int test_no_path(void)
{
    Maze m;
    MazeSolveResult res;
    MazeCell *path = NULL;
    int ok;

    util_test_begin("U07 Labirin tanpa jalur");
    memset(&m, 0, sizeof m);
    maze_load(DEAD_MAZE, &m);
    maze_solve_dfs(&m, &res, &path);
    util_check(res.found == 0 && path == NULL, "U07 DFS tidak menemukan jalur");
    util_free(path);
    path = NULL;
    maze_solve_bfs(&m, &res, &path);
    util_check(res.found == 0 && path == NULL, "U07 BFS tidak menemukan jalur");
    util_free(path);
    path = NULL;
    maze_solve_wall(&m, &res, &path);
    util_check(res.found == 0 && path == NULL, "U07 wall follower tidak menemukan jalur");
    util_free(path);
    maze_destroy(&m);
    ok = util_test_end();
    return ok;
}

/* U08 dan U09: wall follower sukses di labirin sempurna, gagal di labirin putar. */
static int test_wall_follower(void)
{
    Maze m;
    MazeSolveResult res;
    MazeCell *path = NULL;
    int ok;

    util_test_begin("U08 dan U09 Wall follower");
    memset(&m, 0, sizeof m);
    maze_gen_backtracker(&m, 11, 11, 99u, 0);
    maze_solve_wall(&m, &res, &path);
    util_check(res.found == 1, "U08 wall follower berhasil pada labirin sempurna");
    util_free(path);
    path = NULL;
    maze_destroy(&m);
    memset(&m, 0, sizeof m);
    maze_load(LOOP_MAZE, &m);
    maze_solve_wall(&m, &res, &path);
    util_check(res.found == 0, "U09 wall follower gagal pada labirin putar");
    util_free(path);
    path = NULL;
    maze_solve_bfs(&m, &res, &path);
    util_check(res.found == 1 && res.length == 6, "U09 BFS berhasil dengan panjang 6");
    util_free(path);
    maze_destroy(&m);
    ok = util_test_end();
    return ok;
}

/* U10: validator melaporkan sel terisolasi dan E yang tidak terjangkau. */
static int test_validator(void)
{
    Maze m;
    int isolated = -1;
    char msg[128];
    int ok;

    util_test_begin("U10 Validator");
    memset(&m, 0, sizeof m);
    maze_load(ISOLATED_MAZE, &m);
    ok = maze_validate(&m, &isolated, msg, sizeof msg);
    util_check(ok == ST_OK, "U10 terisolasi.txt tetap valid");
    util_check(isolated == 1, "U10 tepat satu sel terisolasi");
    util_check(strstr(msg, "terisolasi") != NULL, "U10 pesan menyebut sel terisolasi");
    maze_destroy(&m);
    memset(&m, 0, sizeof m);
    maze_load(DEAD_MAZE, &m);
    ok = maze_validate(&m, &isolated, msg, sizeof msg);
    util_check(ok == ST_BAD_FORMAT, "U10 E tak terjangkau dilaporkan");
    maze_destroy(&m);
    ok = util_test_end();
    return ok;
}

/* U11 dan U12: perintah editor, undo, dan penolakan perintah yang salah. */
static int test_editor(void)
{
    Maze m;
    MazeUndoStack st;
    char out[UTIL_MAX_LINE];
    int ok;

    util_test_begin("U11 dan U12 Editor");
    memset(&m, 0, sizeof m);
    memset(&st, 0, sizeof st);
    maze_editor_exec(&m, &st, "new 7 7", out, sizeof out);
    util_check(m.grid != NULL && m.h == 7 && m.w == 7, "U11 perintah new membuat labirin");
    util_check(m.grid[1][1] == 'S' && m.grid[5][5] == 'E', "U11 penanda S dan E pada posisi benar");
    maze_editor_exec(&m, &st, "wall 2 2", out, sizeof out);
    util_check(m.grid[2][2] == '#', "U11 perintah wall mengubah sel");
    maze_editor_exec(&m, &st, "undo", out, sizeof out);
    util_check(m.grid[2][2] == '.', "U11 undo mengembalikan sel semula");
    maze_editor_exec(&m, &st, "start 3 3", out, sizeof out);
    util_check(m.grid[3][3] == 'S' && m.start.r == 3, "U11 perintah start memindahkan S");
    maze_editor_exec(&m, &st, "undo", out, sizeof out);
    util_check(m.grid[1][1] == 'S' && m.start.r == 1, "U11 undo mengembalikan posisi S");
    ok = maze_editor_exec(&m, &st, "wall 0 0", out, sizeof out);
    util_check(ok == ST_OUT_OF_RANGE, "U12 sel tepi ditolak");
    ok = maze_editor_exec(&m, &st, "wall 9 9", out, sizeof out);
    util_check(ok == ST_OUT_OF_RANGE, "U12 sel di luar grid ditolak");
    ok = maze_editor_exec(&m, &st, "wall", out, sizeof out);
    util_check(ok == ST_BAD_INPUT, "U12 argumen kurang ditolak");
    ok = maze_editor_exec(&m, &st, "wall a b", out, sizeof out);
    util_check(ok == ST_BAD_INPUT, "U12 argumen bukan angka ditolak");
    ok = maze_editor_exec(&m, &st, "fly 1 1", out, sizeof out);
    util_check(ok == ST_BAD_INPUT, "U12 perintah tak dikenal ditolak");
    ok = maze_editor_exec(&m, &st, "", out, sizeof out);
    util_check(ok == ST_BAD_INPUT, "U12 baris kosong ditolak");
    util_check(m.grid[2][2] == '.' && m.grid[0][0] == '#', "U12 grid tidak berubah");
    maze_undo_stack_free(&st);
    maze_destroy(&m);
    ok = util_test_end();
    return ok;
}

/* Uji tambahan: batas ukuran generator, loops berlebih, gambar jalur, dan labirin besar. */
static int test_extra_core(void)
{
    Maze m;
    MazeSolveResult res;
    MazeCell *path = NULL;
    int ok;

    util_test_begin("Uji tambahan core dan generator");
    memset(&m, 0, sizeof m);
    util_check(maze_gen_backtracker(&m, 4, 5, 1u, 0) == ST_OUT_OF_RANGE, "Ukuran genap ditolak");
    util_check(maze_gen_backtracker(&m, 3, 3, 1u, 0) == ST_OUT_OF_RANGE, "Ukuran 3 ditolak");
    util_check(maze_gen_backtracker(&m, 63, 63, 1u, 0) == ST_OUT_OF_RANGE, "Ukuran 63 ditolak");
    util_check(maze_gen_prim(&m, 11, 11, 5u, 0) == ST_OK, "Prim berukuran valid berhasil");
    util_check(maze_gen_backtracker(&m, 5, 5, 5u, 999) == ST_OK, "loops melebihi kandidat tidak error");
    util_check(maze_gen_backtracker(&m, 61, 61, 7u, 0) == ST_OK, "Labirin 61x61 terbentuk");
    maze_solve_bfs(&m, &res, &path);
    util_check(res.found == 1, "BFS berhasil pada labirin 61x61");
    util_free(path);
    path = NULL;
    maze_solve_dfs(&m, &res, &path);
    util_check(res.found == 1, "DFS berhasil pada labirin 61x61");
    util_free(path);
    path = NULL;
    maze_solve_wall(&m, &res, &path);
    util_check(res.found == 1, "Wall follower berhasil pada labirin 61x61");
    util_free(path);
    path = NULL;
    maze_destroy(&m);
    maze_destroy(&m);
    util_check(m.grid == NULL, "Pemanggilan maze_destroy dua kali aman");
    memset(&m, 0, sizeof m);
    maze_gen_backtracker(&m, 7, 7, 3u, 0);
    maze_solve_bfs(&m, &res, &path);
    maze_draw_path(&m, path, res.length);
    util_check(m.grid[m.start.r][m.start.c] == 'S', "Gambar jalur tidak menimpa S");
    util_check(m.grid[m.end.r][m.end.c] == 'E', "Gambar jalur tidak menimpa E");
    util_free(path);
    path = NULL;
    maze_destroy(&m);
    ok = util_test_end();
    return ok;
}

/* Jalankan seluruh uji modul dasar. */
int test_maze_lab(void)
{
    int failed = 0;

    failed += test_save_load();
    failed += test_broken_files();
    failed += test_same_seed();
    failed += test_perfect_maze();
    failed += test_bfs_length();
    failed += test_no_path();
    failed += test_wall_follower();
    failed += test_validator();
    failed += test_editor();
    failed += test_extra_core();
    failed += maze_test_extra();
    return failed;
}
