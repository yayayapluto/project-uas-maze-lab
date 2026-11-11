#include "maze_lab.h"

#include "../maze_editor/maze_editor.h"
#include "../maze_gen/maze_gen.h"
#include "../maze_solve/maze_solve.h"
#include "../ui/ui.h"

#include <stdio.h>
#include <string.h>

#define MAZE_SEED_MAX 2147483647
#define MAZE_LOOPS_MAX 9999

/* Tampilkan menu utama Maze Lab sebagai panel. */
static void print_main_menu(void)
{
    static const char *const lines[] = {
        "1. Katalog (lihat, urutkan, cari, tambah, buka)",
        "2. Generate labirin",
        "3. Selesaikan labirin (DFS / BFS / wall follower)",
        "4. Editor labirin",
        "5. Benchmark algoritma",
        "6. Simpan labirin aktif",
        "0. Keluar"
    };

    printf("\n");
    ui_panel("MAZE LAB", lines, 7);
}

/* Baca dimensi ganjil antara 5 dan 61, ulang sampai input valid. */
static void read_odd_dims(int *h, int *w)
{
    for (;;) {
        if (util_read_int("Tinggi (5-61, ganjil): ", 5, 61, h) != ST_OK) {
            return;
        }
        if (util_read_int("Lebar (5-61, ganjil): ", 5, 61, w) != ST_OK) {
            return;
        }
        if (*h % 2 == 1 && *w % 2 == 1) {
            return;
        }
        printf("Ukuran harus ganjil.\n");
    }
}

/* Jalankan menu generate: algoritma, ukuran, seed, lalu loops. */
static void menu_generate(Maze *active)
{
    int algo;
    int h;
    int w;
    int seed;
    int loops;
    Maze generated;
    Status st;

    if (util_read_int("Algoritma (1 backtracker, 2 prim): ", 1, 2, &algo) != ST_OK) {
        return;
    }
    read_odd_dims(&h, &w);
    if (util_read_int("Seed (0-2147483647): ", 0, MAZE_SEED_MAX, &seed) != ST_OK) {
        return;
    }
    if (util_read_int("Loops (0-9999): ", 0, MAZE_LOOPS_MAX, &loops) != ST_OK) {
        return;
    }
    memset(&generated, 0, sizeof generated);
    st = (algo == 1) ? maze_gen_backtracker(&generated, h, w, (unsigned)seed, loops)
                     : maze_gen_prim(&generated, h, w, (unsigned)seed, loops);
    if (st != ST_OK) {
        ui_status(st);
        maze_destroy(&generated);
        return;
    }
    maze_print(&generated);
    maze_destroy(active);
    *active = generated;
}

/* Jalankan satu solver pada salinan labirin, lalu tampilkan hasilnya. */
static void solve_with(const Maze *active, const char *solver)
{
    Maze copy;
    MazeSolveResult res;
    MazeCell *path = NULL;
    Status st;

    memset(&copy, 0, sizeof copy);
    if (maze_copy(active, &copy) != ST_OK) {
        printf("Salinan labirin gagal dibuat.\n");
        return;
    }
    if (strcmp(solver, "dfs") == 0) {
        st = maze_solve_dfs(&copy, &res, &path);
    } else if (strcmp(solver, "bfs") == 0) {
        st = maze_solve_bfs(&copy, &res, &path);
    } else {
        st = maze_solve_wall(&copy, &res, &path);
    }
    if (st != ST_OK) {
        ui_status(st);
        util_free(path);
        maze_destroy(&copy);
        return;
    }
    if (res.found) {
        maze_draw_path(&copy, path, res.length);
    }
    maze_print(&copy);
    printf("%s: found=%d, length=%d, visited=%d\n", solver, res.found, res.length, res.visited);
    util_free(path);
    maze_destroy(&copy);
}

/* Kumpulkan hasil ketiga solver untuk tabel perbandingan. */
static void compare_run(const Maze *active, MazeSolveResult *out)
{
    MazeCell *path = NULL;
    int i;

    memset(out, 0, 3 * sizeof *out);
    for (i = 0; i < 3; i++) {
        if (i == 0) {
            maze_solve_dfs(active, &out[0], &path);
        } else if (i == 1) {
            maze_solve_bfs(active, &out[1], &path);
        } else {
            maze_solve_wall(active, &out[2], &path);
        }
        util_free(path);
        path = NULL;
    }
}

/* Tampilkan perbandingan ketiga solver sebagai tabel. */
static void compare_table(const Maze *active, const char *const *solvers)
{
    const char *headers[4] = {"Solver", "Found", "Length", "Visited"};
    const int widths[4] = {8, 6, 6, 8};
    const char *cells[12];
    char num[3][3][12];
    MazeSolveResult res[3];
    int i;

    compare_run(active, res);
    for (i = 0; i < 3; i++) {
        cells[i * 4] = solvers[i];
        snprintf(num[i][0], sizeof num[i][0], "%d", res[i].found);
        snprintf(num[i][1], sizeof num[i][1], "%d", res[i].length);
        snprintf(num[i][2], sizeof num[i][2], "%d", res[i].visited);
        cells[i * 4 + 1] = num[i][0];
        cells[i * 4 + 2] = num[i][1];
        cells[i * 4 + 3] = num[i][2];
    }
    printf("\n");
    ui_table(headers, widths, cells, 3, 4);
}

/* Jalankan menu solve: satu solver atau ketiganya sekaligus. */
static void menu_solve(const Maze *active)
{
    int choice;
    const char *solvers[3];
    int i;

    if (active->grid == NULL) {
        printf("Belum ada labirin aktif.\n");
        return;
    }
    if (util_read_int("Solver (1 DFS, 2 BFS, 3 wall follower, 4 ketiganya): ", 1, 4, &choice) != ST_OK) {
        return;
    }
    solvers[0] = "dfs";
    solvers[1] = "bfs";
    solvers[2] = "wall";
    if (choice == 4) {
        for (i = 0; i < 3; i++) {
            printf("\n--- %s ---\n", solvers[i]);
            solve_with(active, solvers[i]);
        }
        compare_table(active, solvers);
        return;
    }
    solve_with(active, solvers[choice - 1]);
}

/* Simpan labirin aktif ke path yang diketik pengguna. */
static void menu_save(const Maze *active)
{
    char path[UTIL_MAX_PATH];
    Status st;

    if (active->grid == NULL) {
        printf("Belum ada labirin aktif.\n");
        return;
    }
    if (util_read_line("Path berkas: ", path, sizeof path) != ST_OK) {
        return;
    }
    util_trim(path);
    if (path[0] == '\0') {
        printf("Path tidak boleh kosong.\n");
        return;
    }
    st = maze_save(path, active);
    ui_status(st);
}

/* Menjalankan menu utama Maze Lab sampai pengguna memilih keluar. */
void run_maze_lab(void)
{
    Maze active;
    MazeCatalog cat;
    int loaded = 0;
    int running = 1;

    memset(&active, 0, sizeof active);
    memset(&cat, 0, sizeof cat);
    while (running) {
        int choice;
        print_main_menu();
        if (util_read_int("Pilihan: ", 0, 6, &choice) != ST_OK) {
            break;
        }
        switch (choice) {
        case 1:
            maze_menu_catalog(&cat, &loaded, &active);
            break;
        case 2:
            menu_generate(&active);
            break;
        case 3:
            menu_solve(&active);
            break;
        case 4:
            maze_run_editor(&active);
            break;
        case 5:
            maze_menu_bench();
            break;
        case 6:
            menu_save(&active);
            break;
        default:
            running = 0;
            break;
        }
    }
    maze_destroy(&active);
    maze_catalog_free(&cat);
}
