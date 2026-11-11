#include "maze_lab.h"

#include <stdio.h>
#include <string.h>

#define CATALOG_PATH "data/library.txt"

/* Memuat katalog contoh saat submenu katalog pertama kali dibuka. */
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

/* Menampilkan tabel seluruh entri katalog. */
static void show_table(const MazeCatalog *cat)
{
    int i;

    printf("%-4s %-20s %-28s %5s %5s %4s %6s\n", "No", "Nama", "Berkas", "H", "W", "Kes", "Steps");
    for (i = 0; i < cat->count; i++) {
        const MazeInfo *it = &cat->items[i];
        printf("%-4d %-20s %-28s %5d %5d %4d %6d\n", i + 1, it->name, it->file,
               it->h, it->w, it->difficulty, it->best_steps);
    }
    if (cat->count == 0) {
        printf("(katalog kosong)\n");
    }
}

/* Menjalankan pencarian nama dengan metode yang dipilih pengguna. */
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

/* Menjalankan penambahan satu entri katalog. */
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
    printf("%s\n", util_status_message(st));
}

/* Membuka berkas entri katalog sebagai labirin aktif. */
static void open_entry(const MazeCatalog *cat, Maze *active)
{
    int index;
    Maze loaded;
    Status st;

    if (cat->count == 0) {
        printf("Katalog kosong.\n");
        return;
    }
    if (util_read_int("Nomor entri: ", 1, cat->count, &index) != ST_OK) {
        return;
    }
    memset(&loaded, 0, sizeof loaded);
    st = maze_load(cat->items[index - 1].file, &loaded);
    if (st != ST_OK) {
        printf("%s\n", util_status_message(st));
        maze_destroy(&loaded);
        return;
    }
    maze_destroy(active);
    *active = loaded;
    printf("Labirin %s dimuat sebagai labirin aktif.\n", cat->items[index - 1].name);
}

/* Menyimpan katalog ke path yang diketik pengguna. */
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
    printf("%s\n", util_status_message(st));
}

/* Menjalankan submenu katalog sampai pengguna memilih kembali. */
void maze_menu_catalog(MazeCatalog *cat, int *loaded, Maze *active)
{
    ensure_loaded(cat, loaded);
    for (;;) {
        int choice;
        printf("\n=== KATALOG ===\n");
        printf("1. Lihat tabel\n");
        printf("2. Urut ukuran\n");
        printf("3. Urut kesulitan\n");
        printf("4. Urut nama\n");
        printf("5. Cari sequential\n");
        printf("6. Cari binary\n");
        printf("7. Tambah entri\n");
        printf("8. Buka labirin\n");
        printf("9. Simpan katalog\n");
        printf("0. Kembali\n");
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
