#include "maze_lab.h"

#include "../ui/ui.h"

#include <stdio.h>
#include <string.h>

#define CATALOG_PATH "data/library.txt"

/* Muat katalog contoh saat submenu katalog dibuka pertama kali. */
static void ensure_loaded(MazeCatalog *cat, int *loaded)
{
    int bad = 0;
    Status st;

    if (*loaded) {
        return;
    }
    *loaded = 1;
    st = maze_catalog_load(CATALOG_PATH, cat, &bad);
    if (st != ST_OK) {
        printf("Katalog %s gagal dimuat: %s\n", CATALOG_PATH, util_status_message(st));
        printf("Lanjut dengan katalog kosong.\n");
        return;
    }
    printf("Katalog dimuat: %d entri, %d baris rusak.\n", cat->count, bad);
}

/* Tampilkan tabel semua entri katalog. */
static void show_table(const MazeCatalog *cat)
{
    const char *headers[7] = {"No", "Nama", "File", "H", "W", "Diff", "Steps"};
    const int widths[7] = {4, 20, 28, 5, 5, 5, 6};
    const char **cells;
    char (*num)[5][12];
    int i;

    if (cat->count == 0) {
        printf("(katalog kosong)\n");
        return;
    }
    cells = (const char **)util_calloc((size_t)cat->count * 7, sizeof(char *));
    num = (char (*)[5][12])util_malloc((size_t)cat->count * sizeof *num);
    for (i = 0; i < cat->count; i++) {
        const MazeInfo *it = &cat->items[i];
        size_t base = (size_t)i * 7;

        snprintf(num[i][0], sizeof num[i][0], "%d", i + 1);
        snprintf(num[i][1], sizeof num[i][1], "%d", it->h);
        snprintf(num[i][2], sizeof num[i][2], "%d", it->w);
        snprintf(num[i][3], sizeof num[i][3], "%d", it->difficulty);
        snprintf(num[i][4], sizeof num[i][4], "%d", it->best_steps);
        cells[base + 0] = num[i][0];
        cells[base + 1] = it->name;
        cells[base + 2] = it->file;
        cells[base + 3] = num[i][1];
        cells[base + 4] = num[i][2];
        cells[base + 5] = num[i][3];
        cells[base + 6] = num[i][4];
    }
    ui_table(headers, widths, cells, cat->count, 7);
    util_free(num);
    util_free(cells);
}

/* Jalankan pencarian nama sesuai metode yang dipilih pengguna. */
static void find_name(const MazeCatalog *cat, int binary)
{
    char name[UTIL_MAX_LINE];
    int idx;

    if (util_read_line("Nama yang dicari: ", name, sizeof name) != ST_OK) {
        return;
    }
    util_trim(name);
    idx = binary ? maze_catalog_find_name_binary(cat, name) : maze_catalog_find_name_seq(cat, name);
    if (idx == -2) {
        printf("Katalog belum terurut nama. Urutkan nama lebih dulu.\n");
        return;
    }
    if (idx < 0) {
        printf("Nama tidak ditemukan.\n");
        return;
    }
    printf("Ditemukan pada indeks %d: %s (%s)\n", idx + 1, cat->items[idx].name, cat->items[idx].file);
}

/* Tambahkan satu entri katalog. */
static void add_entry(MazeCatalog *cat)
{
    MazeInfo info;
    char name[UTIL_MAX_LINE];
    char file[UTIL_MAX_PATH];
    Status st;

    memset(&info, 0, sizeof info);
    if (util_read_line("Nama: ", name, sizeof name) != ST_OK) {
        return;
    }
    util_trim(name);
    if (util_read_line("Berkas: ", file, sizeof file) != ST_OK) {
        return;
    }
    util_trim(file);
    if (util_read_int("Tinggi (5-61): ", 5, 61, &info.h) != ST_OK) {
        return;
    }
    if (util_read_int("Lebar (5-61): ", 5, 61, &info.w) != ST_OK) {
        return;
    }
    if (util_read_int("Kesulitan (1-5): ", 1, 5, &info.difficulty) != ST_OK) {
        return;
    }
    if (util_read_int("Best steps (-1 bila belum diketahui): ", -1, 1000000, &info.best_steps) != ST_OK) {
        return;
    }
    if (strlen(name) >= sizeof info.name) {
        printf("Nama terlalu panjang, maksimal 31 karakter.\n");
        return;
    }
    if (strlen(file) >= sizeof info.file) {
        printf("Nama berkas terlalu panjang.\n");
        return;
    }
    memcpy(info.name, name, strlen(name) + 1);
    memcpy(info.file, file, strlen(file) + 1);
    st = maze_catalog_add(cat, &info);
    ui_status(st);
}

/* Buka file entri katalog sebagai labirin aktif. */
static void open_entry(const MazeCatalog *cat, Maze *active)
{
    int index;
    Maze loaded;
    Status st;

    if (cat->count == 0) {
        printf("Katalog kosong.\n");
        return;
    }    if (util_read_int("Nomor entri: ", 1, cat->count, &index) != ST_OK) {
        return;
    }
    memset(&loaded, 0, sizeof loaded);
    st = maze_load(cat->items[index - 1].file, &loaded);
    if (st != ST_OK) {
        ui_status(st);
        maze_destroy(&loaded);
        return;
    }
    maze_destroy(active);
    *active = loaded;
    printf("Labirin %s dimuat sebagai labirin aktif.\n", cat->items[index - 1].name);
}

/* Simpan katalog ke path yang diketik pengguna. */
static void save_catalog(const MazeCatalog *cat)
{
    char path[UTIL_MAX_PATH];
    Status st;

    if (util_read_line("Path berkas katalog: ", path, sizeof path) != ST_OK) {
        return;
    }
    util_trim(path);
    if (path[0] == '\0') {
        printf("Path tidak boleh kosong.\n");
        return;
    }
    st = maze_catalog_save(path, cat);
    ui_status(st);
}

/* Tampilkan submenu katalog sebagai panel. */
static void print_catalog_menu(void)
{
    static const char *const lines[] = {
        "1. Lihat tabel",
        "2. Urut ukuran",
        "3. Urut kesulitan",
        "4. Urut nama",
        "5. Cari sequential",
        "6. Cari binary",
        "7. Tambah entri",
        "8. Buka labirin",
        "9. Simpan katalog",
        "0. Kembali"
    };

    printf("\n");
    ui_panel("KATALOG", lines, 10);
}

/* Jalankan submenu katalog sampai pengguna memilih kembali. */
void maze_menu_catalog(MazeCatalog *cat, int *loaded, Maze *active)
{
    ensure_loaded(cat, loaded);
    for (;;) {
        int choice;
        print_catalog_menu();
        if (util_read_int("Pilihan: ", 0, 9, &choice) != ST_OK) {
            return;
        }
        if (choice == 0) {
            return;
        }
        switch (choice) {
        case 1: show_table(cat); break;
        case 2: maze_catalog_sort_by_size(cat); break;
        case 3: maze_catalog_sort_by_difficulty(cat); break;
        case 4: maze_catalog_sort_by_name(cat); break;
        case 5: find_name(cat, 0); break;
        case 6: find_name(cat, 1); break;
        case 7: add_entry(cat); break;
        case 8: open_entry(cat, active); break;
        default: save_catalog(cat); break;
        }
    }
}
