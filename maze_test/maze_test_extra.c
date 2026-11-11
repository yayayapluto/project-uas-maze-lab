#include "../maze_lab/maze_lab.h"

#include "../maze_editor/maze_editor.h"
#include "../maze_gen/maze_gen.h"
#include "../maze_solve/maze_solve.h"
#include "maze_test.h"

#include <stdio.h>
#include <string.h>

#define LIBRARY_PATH "data/library.txt"
#define BROKEN_CATALOG "data/katalog_rusak.txt"
#define EXPECTED_BAD_LINES 5

/* Isi satu entri katalog untuk keperluan uji. */
static void make_info(MazeInfo *info, const char *name, int h, int w, int difficulty, int best)
{
    memset(info, 0, sizeof *info);
    snprintf(info->name, sizeof info->name, "%s", name);
    snprintf(info->file, sizeof info->file, "data/%s.txt", name);
    info->h = h;
    info->w = w;
    info->difficulty = difficulty;
    info->best_steps = best;
}

/* U13: urutkan katalog berdasarkan ukuran, kesulitan, dan nama. */
static int test_catalog_sort(void)
{
    MazeCatalog cat;
    MazeInfo info;
    int ok;

    util_test_begin("U13 Urutan katalog");
    memset(&cat, 0, sizeof cat);
    make_info(&info, "besar", 11, 21, 3, 5);
    maze_catalog_add(&cat, &info);
    make_info(&info, "kecil", 5, 5, 1, 2);
    maze_catalog_add(&cat, &info);
    make_info(&info, "sedang", 7, 9, 2, 3);
    maze_catalog_add(&cat, &info);
    maze_catalog_sort_by_size(&cat);
    util_check(cat.items[0].h == 5 && cat.items[2].h == 11, "U13 urut ukuran menaik");
    util_check(cat.sorted_by_name == 0, "U13 urut ukuran mengosongkan penanda nama");
    maze_catalog_sort_by_difficulty(&cat);
    util_check(cat.items[0].difficulty == 1 && cat.items[2].difficulty == 3, "U13 urut kesulitan menaik");
    util_check(cat.sorted_by_name == 0, "U13 urut kesulitan mengosongkan penanda nama");
    maze_catalog_sort_by_name(&cat);
    util_check(strcmp(cat.items[0].name, "besar") == 0, "U13 urut nama menaik");
    util_check(strcmp(cat.items[1].name, "kecil") == 0, "U13 urut nama kedua benar");
    util_check(strcmp(cat.items[2].name, "sedang") == 0, "U13 urut nama ketiga benar");
    util_check(cat.sorted_by_name == 1, "U13 urut nama mengisi penanda nama");
    maze_catalog_free(&cat);
    ok = util_test_end();
    return ok;
}

/* U14: binary search mengembalikan -2 sebelum urutan nama dan benar sesudahnya. */
static int test_catalog_search(void)
{
    MazeCatalog cat;
    MazeInfo info;
    int idx;

    util_test_begin("U14 Binary search katalog");
    memset(&cat, 0, sizeof cat);
    make_info(&info, "apel", 5, 5, 1, 2);
    maze_catalog_add(&cat, &info);
    make_info(&info, "jeruk", 7, 7, 2, 3);
    maze_catalog_add(&cat, &info);
    make_info(&info, "mangga", 9, 9, 3, 4);
    maze_catalog_add(&cat, &info);
    idx = maze_catalog_find_name_binary(&cat, "jeruk");
    util_check(idx == -2, "U14 binary search menolak katalog belum terurut");
    util_check(maze_catalog_find_name_seq(&cat, "MANGGA") == 2, "U14 sequential search menemukan nama");
    util_check(maze_catalog_find_name_seq(&cat, "tidak ada") == -1, "U14 sequential search mengembalikan -1");
    maze_catalog_sort_by_name(&cat);
    idx = maze_catalog_find_name_binary(&cat, "jeruk");
    util_check(idx >= 0 && strcmp(cat.items[idx].name, "jeruk") == 0, "U14 binary search menemukan nama");
    util_check(maze_catalog_find_name_binary(&cat, "tidak ada") == -1, "U14 binary search mengembalikan -1");
    maze_catalog_free(&cat);
    return util_test_end();
}

/* U15: baris rusak pada katalog dihitung, tapi pemuatan tetap jalan. */
static int test_catalog_broken(void)
{
    MazeCatalog cat;
    int bad = -1;
    Status st;

    util_test_begin("U15 Katalog rusak");
    memset(&cat, 0, sizeof cat);
    st = maze_catalog_load(BROKEN_CATALOG, &cat, &bad);
    util_check(st == ST_OK, "U15 katalog rusak tetap termuat");
    util_check(bad == EXPECTED_BAD_LINES, "U15 jumlah baris rusak dilaporkan");
    util_check(cat.count > 0, "U15 entri sah tetap termuat");
    util_check(cat.sorted_by_name == 0, "U15 katalog hasil muat belum terurut nama");
    maze_catalog_free(&cat);
    return util_test_end();
}

/* U16: jumlah baris benchmark harus n x 2 generator x 3 solver. */
static int test_bench_count(void)
{
    MazeBenchRow *rows = NULL;
    int count = 0;

    util_test_begin("U16 Jumlah baris benchmark");
    util_check(maze_bench_run(1, 100u, 5, 5, 0, &rows, &count) == ST_OK, "U16 benchmark n=1 berjalan");
    util_check(count == 6, "U16 n=1 menghasilkan enam baris");
    util_free(rows);
    rows = NULL;
    util_check(maze_bench_run(3, 100u, 7, 7, 0, &rows, &count) == ST_OK, "U16 benchmark n=3 berjalan");
    util_check(count == 18, "U16 n=3 menghasilkan delapan belas baris");
    util_free(rows);
    return util_test_end();
}

/* U17: statistik ringkasan harus cocok dengan hitungan manual. */
static int test_bench_stats(void)
{
    MazeBenchRow *rows = NULL;
    int count = 0;
    MazeBenchStat st;
    int tries = 0;
    int ok_count = 0;
    int sum = 0;
    int lo = 0;
    int hi = 0;
    long visited = 0;
    int i;

    util_test_begin("U17 Ringkasan benchmark");
    maze_bench_run(2, 50u, 5, 7, 0, &rows, &count);
    for (i = 0; i < count; i++) {
        if (strcmp(rows[i].gen, "backtracker") != 0 || strcmp(rows[i].solver, "bfs") != 0) {
            continue;
        }
        tries++;
        visited += rows[i].visited;
        if (rows[i].found) {
            ok_count++;
            sum += rows[i].length;
            if (ok_count == 1) {
                lo = rows[i].length;
                hi = rows[i].length;
            } else {
                if (rows[i].length < lo) {
                    lo = rows[i].length;
                }
                if (rows[i].length > hi) {
                    hi = rows[i].length;
                }
            }
        }
    }
    maze_bench_stats(rows, count, "backtracker", "bfs", &st);
    util_check(st.tries == tries, "U17 jumlah percobaan sesuai hitungan manual");
    util_check(st.ok == ok_count, "U17 jumlah berhasil sesuai hitungan manual");
    util_check(st.min_length == lo && st.max_length == hi, "U17 panjang minimum dan maksimum sesuai");
    util_check_double(st.avg_length, (double)sum / (double)ok_count, 1e-9, "U17 rata-rata panjang sesuai");
    util_check_double(st.avg_visited, (double)visited / (double)tries, 1e-9, "U17 rata-rata dikunjungi sesuai");
    util_check_double(st.fail_pct, 0.0, 1e-9, "U17 persentase gagal sesuai");
    util_free(rows);
    return util_test_end();
}

/* U18: pencarian seed pada baris benchmark. */
static int test_bench_seed(void)
{
    MazeBenchRow *rows = NULL;
    int count = 0;
    int idx;

    util_test_begin("U18 Pencarian seed benchmark");
    maze_bench_run(2, 300u, 5, 5, 0, &rows, &count);
    idx = maze_bench_find_seed(rows, count, 301u);
    util_check(idx >= 0 && rows[idx].seed == 301u, "U18 seed yang ada ditemukan");
    util_check(maze_bench_find_seed(rows, count, 9999u) == -1, "U18 seed yang tidak ada menghasilkan -1");
    util_free(rows);
    return util_test_end();
}

/* U19: kegagalan alokasi di maze_create dan generator tidak boleh meninggalkan kebocoran. */
static int test_alloc_failures(void)
{
    Maze m;
    int k;
    Status st;

    util_test_begin("U19 Kegagalan alokasi");
    for (k = 0; k <= 3; k++) {
        memset(&m, 0, sizeof m);
        util_fail_alloc_after(k);
        st = maze_create(&m, 9, 9);
        util_check(st == ST_NO_MEMORY, "maze_create gagal alokasi");
        util_check(util_alloc_balance() == 0, "keseimbangan alokasi nol setelah maze_create gagal");
        util_fail_alloc_after(-1);
        maze_destroy(&m);
    }
    memset(&m, 0, sizeof m);
    util_fail_alloc_after(6);
    st = maze_gen_prim(&m, 5, 5, 1u, 0);
    util_check(st == ST_NO_MEMORY, "generator prim gagal alokasi");
    util_check(util_alloc_balance() == 0, "keseimbangan alokasi nol setelah prim gagal");
    util_fail_alloc_after(-1);
    maze_destroy(&m);
    memset(&m, 0, sizeof m);
    util_fail_alloc_after(7);
    st = maze_gen_backtracker(&m, 5, 5, 1u, 5);
    util_check(st == ST_NO_MEMORY, "pembukaan loops gagal alokasi");
    util_check(util_alloc_balance() == 0, "keseimbangan alokasi nol setelah loops gagal");
    util_fail_alloc_after(-1);
    maze_destroy(&m);
    return util_test_end();
}

/* Kegagalan alokasi saat benchmark, plus verifikasi file katalog contoh. */
static int test_bench_alloc_and_library(void)
{
    MazeBenchRow *rows = NULL;
    int count = 0;
    MazeCatalog cat;
    int bad = 0;
    int i;
    int ok = 1;

    util_test_begin("Uji tambahan benchmark dan katalog contoh");
    util_fail_alloc_after(1);
    util_check(maze_bench_run(2, 10u, 5, 5, 0, &rows, &count) == ST_NO_MEMORY,
               "benchmark gagal alokasi dilaporkan");
    util_fail_alloc_after(-1);
    util_check(util_alloc_balance() == 0, "keseimbangan alokasi nol setelah benchmark gagal");
    memset(&cat, 0, sizeof cat);
    util_check(maze_catalog_load(LIBRARY_PATH, &cat, &bad) == ST_OK, "library.txt termuat");
    util_check(bad == 0, "library.txt tidak memuat baris rusak");
    util_check(cat.count >= 10, "library.txt memuat minimal sepuluh entri");
    for (i = 0; i < cat.count; i++) {
        Maze m;
        MazeSolveResult res;
        MazeCell *path = NULL;
        memset(&m, 0, sizeof m);
        if (maze_load(cat.items[i].file, &m) != ST_OK) {
            util_check(0, "file entri katalog termuat");
            maze_destroy(&m);
            continue;
        }
        util_check(m.h == cat.items[i].h && m.w == cat.items[i].w, "ukuran entri sesuai file");
        maze_solve_bfs(&m, &res, &path);
        if (cat.items[i].best_steps == -1) {
            util_check(res.found == 0, "entri tanpa jalur sesuai");
        } else {
            util_check(res.found == 1 && res.length == cat.items[i].best_steps,
                       "best_steps sama dengan panjang BFS");
        }
        util_free(path);
        maze_destroy(&m);
    }
    maze_catalog_free(&cat);
    ok = util_test_end();
    return ok;
}

/* Uji ketahanan: muat file berakhiran CRLF, dan editor tanpa labirin aktif. */
static int test_crlf_and_editor_states(void)
{
    static const char content[] = "5 5\r\n#####\r\n#S..#\r\n#.#.#\r\n#..E#\r\n#####\r\n";
    const char *path = "data/tmp_crlf.txt";
    FILE *f;
    Maze m;
    MazeUndoStack st;
    char out[UTIL_MAX_LINE];
    Status st2;

    util_test_begin("Uji ketahanan CRLF dan editor tanpa labirin");
    f = fopen(path, "wb");
    if (f == NULL) {
        util_check(0, "file sementara CRLF dapat dibuat");
        return util_test_end();
    }
    fwrite(content, 1, sizeof content - 1, f);
    fclose(f);
    memset(&m, 0, sizeof m);
    util_check(maze_load(path, &m) == ST_OK, "file CRLF termuat");
    util_check(m.grid[1][1] == 'S' && m.grid[3][3] == 'E', "penanda S dan E pada file CRLF");
    maze_destroy(&m);
    remove(path);
    memset(&m, 0, sizeof m);
    memset(&st, 0, sizeof st);
    st2 = maze_editor_exec(&m, &st, "wall 1 1", out, sizeof out);
    util_check(st2 == ST_BAD_INPUT, "editor tanpa labirin menolak perintah ubah");
    util_check(strstr(out, "new") != NULL, "pesan editor menyarankan new atau load");
    st2 = maze_editor_exec(&m, &st, "help", out, sizeof out);
    util_check(st2 == ST_OK, "editor tanpa labirin menerima help");
    maze_undo_stack_free(&st);
    maze_destroy(&m);
    return util_test_end();
}

/* Jalankan kelompok uji katalog, benchmark, alokasi, dan data contoh. */
int maze_test_extra(void)
{
    int failed = 0;

    failed += test_catalog_sort();
    failed += test_catalog_search();
    failed += test_catalog_broken();
    failed += test_bench_count();
    failed += test_bench_stats();
    failed += test_bench_seed();
    failed += test_alloc_failures();
    failed += test_bench_alloc_and_library();
    failed += test_crlf_and_editor_states();
    util_test_begin("U20 Penutup");
    util_check(util_alloc_balance() == 0, "U20 keseimbangan alokasi nol");
    failed += util_test_end();
    return failed;
}
