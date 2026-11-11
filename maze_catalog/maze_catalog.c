#include "maze_catalog.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CATALOG_FIELDS 6
#define CATALOG_MIN_DIM 5
#define CATALOG_MAX_DIM 61

/* Pecah satu baris katalog menjadi kolom; kembalikan jumlah kolomnya. */
static int split_fields(char *line, char *fields[], int max)
{
    int count = 0;
    char *p = line;

    for (;;) {
        char *sep = strchr(p, ';');
        if (count < max) {
            fields[count] = p;
        }
        count++;
        if (sep == NULL) {
            break;
        }
        *sep = '\0';
        p = sep + 1;
    }
    return count;
}

/* Parse satu kolom menjadi bilangan bulat; gagal kalau bukan angka utuh. */
static int parse_int(const char *s, int *out)
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

/* Periksa satu baris katalog; kalau sah, isi struct info-nya. */
static int parse_line(char *line, MazeInfo *info)
{
    char *fields[CATALOG_FIELDS + 2];
    int n = split_fields(line, fields, CATALOG_FIELDS + 2);
    int h;
    int w;
    int difficulty;
    int best_steps;

    if (n != CATALOG_FIELDS) {
        return 0;
    }
    util_trim(fields[0]);
    util_trim(fields[1]);
    if (fields[0][0] == '\0' || strlen(fields[0]) >= sizeof info->name) {
        return 0;
    }
    if (fields[1][0] == '\0' || strlen(fields[1]) >= sizeof info->file) {
        return 0;
    }
    if (!parse_int(fields[2], &h) || !parse_int(fields[3], &w)) {
        return 0;
    }
    if (!parse_int(fields[4], &difficulty) || !parse_int(fields[5], &best_steps)) {
        return 0;
    }
    if (h < CATALOG_MIN_DIM || h > CATALOG_MAX_DIM || w < CATALOG_MIN_DIM || w > CATALOG_MAX_DIM) {
        return 0;
    }
    if (difficulty < 1 || difficulty > 5) {
        return 0;
    }
    if (best_steps < -1) {
        return 0;
    }
    memset(info, 0, sizeof *info);
    strcpy(info->name, fields[0]);
    strcpy(info->file, fields[1]);
    info->h = h;
    info->w = w;
    info->difficulty = difficulty;
    info->best_steps = best_steps;
    return 1;
}

/* Perbesar array entri katalog kalau kapasitasnya sudah penuh. */
static Status grow_catalog(MazeCatalog *cat)
{
    int newcap = (cat->capacity == 0) ? 4 : cat->capacity * 2;
    MazeInfo *tmp = (MazeInfo *)util_realloc(cat->items, (size_t)newcap * sizeof(MazeInfo));

    if (tmp == NULL) {
        return ST_NO_MEMORY;
    }
    cat->items = tmp;
    cat->capacity = newcap;
    return ST_OK;
}

/* Ambil baris berikutnya dari buffer; kembalikan 1 kalau masih ada. */
static int next_line(char **cur, char **line_out)
{
    char *line = *cur;
    char *nl = strchr(line, '\n');

    if (nl != NULL) {
        *nl = '\0';
        *cur = nl + 1;
    } else {
        if (line[0] == '\0') {
            return 0;
        }
        *cur = line + strlen(line);
    }
    line[strcspn(line, "\r")] = '\0';
    *line_out = line;
    return 1;
}

/* Tambah satu entri ke array katalog, besarkan kapasitasnya kalau perlu. */
static Status append_info(MazeCatalog *cat, const MazeInfo *info)
{
    if (cat->count == cat->capacity) {
        Status st = grow_catalog(cat);
        if (st != ST_OK) {
            return st;
        }
    }
    cat->items[cat->count++] = *info;
    return ST_OK;
}

/* Muat katalog dari file; baris yang rusak hanya dihitung, pemuatan tetap jalan. */
Status maze_catalog_load(const char *path, MazeCatalog *cat, int *bad_lines)
{
    char *data = NULL;
    size_t len = 0;
    char *cur;
    char *line;
    Status st;
    int bad = 0;

    if (path == NULL || cat == NULL) {
        return ST_BAD_INPUT;
    }
    st = util_read_file(path, &data, &len);
    if (st != ST_OK) {
        return st;
    }
    cur = data;
    while (next_line(&cur, &line)) {
        MazeInfo info;
        if (line[0] == '\0') {
            continue;
        }
        if (!parse_line(line, &info)) {
            bad++;
            continue;
        }
        st = append_info(cat, &info);
        if (st != ST_OK) {
            util_free(data);
            maze_catalog_free(cat);
            return st;
        }
    }
    util_free(data);
    cat->sorted_by_name = 0;
    if (bad_lines != NULL) {
        *bad_lines = bad;
    }
    return ST_OK;
}

/* Simpan katalog ke file teks. */
Status maze_catalog_save(const char *path, const MazeCatalog *cat)
{
    char *text;
    size_t cap;
    size_t used = 0;
    int i;
    Status st;

    if (path == NULL || cat == NULL) {
        return ST_BAD_INPUT;
    }
    cap = (size_t)(cat->count + 1) * (sizeof(MazeInfo) + 8);
    text = (char *)util_malloc(cap);
    if (text == NULL) {
        return ST_NO_MEMORY;
    }
    for (i = 0; i < cat->count; i++) {
        const MazeInfo *it = &cat->items[i];
        int written = snprintf(text + used, cap - used, "%s;%s;%d;%d;%d;%d\n",
                               it->name, it->file, it->h, it->w, it->difficulty, it->best_steps);
        if (written < 0 || (size_t)written >= cap - used) {
            util_free(text);
            return ST_NO_MEMORY;
        }
        used += (size_t)written;
    }
    st = util_write_file(path, text, used);
    util_free(text);
    return st;
}

/* Tambah satu entri ke katalog; urutan nama otomatis jadi tidak sah lagi. */
Status maze_catalog_add(MazeCatalog *cat, const MazeInfo *info)
{
    if (cat == NULL || info == NULL) {
        return ST_BAD_INPUT;
    }
    if (info->name[0] == '\0' || info->file[0] == '\0') {
        return ST_BAD_INPUT;
    }
    if (info->h < CATALOG_MIN_DIM || info->h > CATALOG_MAX_DIM ||
        info->w < CATALOG_MIN_DIM || info->w > CATALOG_MAX_DIM) {
        return ST_OUT_OF_RANGE;
    }
    if (info->difficulty < 1 || info->difficulty > 5) {
        return ST_OUT_OF_RANGE;
    }
    if (cat->count == cat->capacity) {
        Status st = grow_catalog(cat);
        if (st != ST_OK) {
            return st;
        }
    }
    cat->items[cat->count++] = *info;
    cat->sorted_by_name = 0;
    return ST_OK;
}

/* Urutkan katalog berdasarkan ukuran sel dengan Bubble Sort. */
void maze_catalog_sort_by_size(MazeCatalog *cat)
{
    int i;
    int j;

    if (cat == NULL) {
        return;
    }
    for (i = 0; i < cat->count - 1; i++) {
        for (j = 0; j < cat->count - 1 - i; j++) {
            int a = cat->items[j].h * cat->items[j].w;
            int b = cat->items[j + 1].h * cat->items[j + 1].w;
            if (a > b) {
                MazeInfo tmp = cat->items[j];
                cat->items[j] = cat->items[j + 1];
                cat->items[j + 1] = tmp;
            }
        }
    }
    cat->sorted_by_name = 0;
}

/* Urutkan katalog berdasarkan kesulitan dengan Selection Sort. */
void maze_catalog_sort_by_difficulty(MazeCatalog *cat)
{
    int i;
    int j;

    if (cat == NULL) {
        return;
    }
    for (i = 0; i < cat->count - 1; i++) {
        int best = i;
        for (j = i + 1; j < cat->count; j++) {
            if (cat->items[j].difficulty < cat->items[best].difficulty) {
                best = j;
            }
        }
        if (best != i) {
            MazeInfo tmp = cat->items[i];
            cat->items[i] = cat->items[best];
            cat->items[best] = tmp;
        }
    }
    cat->sorted_by_name = 0;
}

/* Urutkan katalog berdasarkan nama dengan Selection Sort. */
void maze_catalog_sort_by_name(MazeCatalog *cat)
{
    int i;
    int j;

    if (cat == NULL) {
        return;
    }
    for (i = 0; i < cat->count - 1; i++) {
        int best = i;
        for (j = i + 1; j < cat->count; j++) {
            if (util_stricmp(cat->items[j].name, cat->items[best].name) < 0) {
                best = j;
            }
        }
        if (best != i) {
            MazeInfo tmp = cat->items[i];
            cat->items[i] = cat->items[best];
            cat->items[best] = tmp;
        }
    }
    cat->sorted_by_name = 1;
}

/* Cari nama secara sequential; kembalikan indeks atau -1. */
int maze_catalog_find_name_seq(const MazeCatalog *cat, const char *name)
{
    int i;

    if (cat == NULL || name == NULL) {
        return -1;
    }
    for (i = 0; i < cat->count; i++) {
        if (util_stricmp(cat->items[i].name, name) == 0) {
            return i;
        }
    }
    return -1;
}

/* Cari nama dengan binary search; kembalikan -2 kalau katalog belum diurutkan nama. */
int maze_catalog_find_name_binary(const MazeCatalog *cat, const char *name)
{
    int low = 0;
    int high;

    if (cat == NULL || name == NULL) {
        return -1;
    }
    if (cat->sorted_by_name == 0) {
        return -2;
    }
    high = cat->count - 1;
    while (low <= high) {
        int mid = low + (high - low) / 2;
        int cmp = util_stricmp(cat->items[mid].name, name);
        if (cmp == 0) {
            return mid;
        }
        if (cmp < 0) {
            low = mid + 1;
        } else {
            high = mid - 1;
        }
    }
    return -1;
}

/* Lepas seluruh isi katalog dan kembalikan ke keadaan kosong. */
void maze_catalog_free(MazeCatalog *cat)
{
    if (cat == NULL) {
        return;
    }
    util_free(cat->items);
    cat->items = NULL;
    cat->count = 0;
    cat->capacity = 0;
    cat->sorted_by_name = 0;
}
