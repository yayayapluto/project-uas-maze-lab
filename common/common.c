#include "common.h"

#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static long g_alloc_count = 0;
static long g_free_count = 0;
static int g_fail_after = -1;
static int g_checks_total = 0;
static int g_checks_failed = 0;

/* Menyatakan alokasi berikutnya harus gagal atau tidak. */
static int alloc_should_fail(void)
{
    if (g_fail_after < 0) {
        return 0;
    }
    if (g_fail_after > 0) {
        g_fail_after--;
        return 0;
    }
    return 1;
}

/* Mengembalikan pesan bahasa Indonesia untuk setiap status. */
const char *util_status_message(Status s)
{
    switch (s) {
    case ST_OK:            return "Berhasil.";
    case ST_FILE_NOT_FOUND: return "Berkas tidak ditemukan.";
    case ST_FILE_EMPTY:    return "Berkas kosong.";
    case ST_FILE_WRITE:    return "Gagal menulis berkas.";
    case ST_BAD_INPUT:     return "Masukan tidak valid.";
    case ST_BAD_FORMAT:    return "Format tidak sesuai.";
    case ST_OUT_OF_RANGE:  return "Nilai di luar rentang yang diperbolehkan.";
    case ST_NO_MEMORY:     return "Memori tidak cukup.";
    default:               return "Status tidak dikenal.";
    }
}

/* Membuang spasi di tepi string. */
void util_trim(char *s)
{
    size_t start = 0;
    size_t end;

    if (s == NULL) {
        return;
    }
    end = strlen(s);
    while (s[start] != '\0' && isspace((unsigned char)s[start])) {
        start++;
    }
    while (end > start && isspace((unsigned char)s[end - 1])) {
        end--;
    }
    if (start > 0) {
        memmove(s, s + start, end - start);
    }
    s[end - start] = '\0';
}

/* Mengubah seluruh huruf besar menjadi huruf kecil. */
void util_to_lower(char *s)
{
    size_t i;

    if (s == NULL) {
        return;
    }
    for (i = 0; s[i] != '\0'; i++) {
        s[i] = (char)tolower((unsigned char)s[i]);
    }
}

/* Membandingkan dua string tanpa membedakan huruf besar dan kecil. */
int util_stricmp(const char *a, const char *b)
{
    size_t i = 0;

    if (a == NULL || b == NULL) {
        return (a == b) ? 0 : 1;
    }
    while (a[i] != '\0' && b[i] != '\0') {
        int ca = tolower((unsigned char)a[i]);
        int cb = tolower((unsigned char)b[i]);
        if (ca != cb) {
            return ca - cb;
        }
        i++;
    }
    return (int)a[i] - (int)b[i];
}

/* Membaca satu baris; sisa baris terlalu panjang dibuang agar tidak tertinggal. */
Status util_read_line(const char *prompt, char *buf, size_t size)
{
    int ch;

    if (buf == NULL || size == 0) {
        return ST_BAD_INPUT;
    }
    if (prompt != NULL) {
        printf("%s", prompt);
        fflush(stdout);
    }
    if (fgets(buf, (int)size, stdin) == NULL) {
        buf[0] = '\0';
        return ST_BAD_INPUT;
    }
    if (strchr(buf, '\n') == NULL) {
        while ((ch = fgetc(stdin)) != EOF && ch != '\n') {
            continue;
        }
    }
    buf[strcspn(buf, "\r\n")] = '\0';
    return ST_OK;
}

/* Membaca bilangan bulat dalam rentang, mengulang sampai valid atau EOF. */
Status util_read_int(const char *prompt, int min, int max, int *out)
{
    char buf[UTIL_MAX_LINE];

    if (out == NULL) {
        return ST_BAD_INPUT;
    }
    for (;;) {
        char *end = NULL;
        long value;
        Status s = util_read_line(prompt, buf, sizeof buf);
        if (s != ST_OK) {
            return s;
        }
        util_trim(buf);
        if (buf[0] == '\0') {
            printf("Masukkan angka.\n");
            continue;
        }
        value = strtol(buf, &end, 10);
        if (end == buf || *end != '\0') {
            printf("Masukkan bilangan bulat yang valid.\n");
            continue;
        }
        if (value < (long)min || value > (long)max) {
            printf("Nilai harus antara %d dan %d.\n", min, max);
            continue;
        }
        *out = (int)value;
        return ST_OK;
    }
}

/* Alokasi satu blok dan mencatatnya pada pelacak alokasi. */
void *util_malloc(size_t n)
{
    void *p;

    if (alloc_should_fail()) {
        return NULL;
    }
    p = malloc(n);
    if (p != NULL) {
        g_alloc_count++;
    }
    return p;
}

/* Alokasi banyak elemen dan menginisialisasinya dengan nol. */
void *util_calloc(size_t n, size_t size)
{
    void *p;

    if (alloc_should_fail()) {
        return NULL;
    }
    p = calloc(n, size);
    if (p != NULL) {
        g_alloc_count++;
    }
    return p;
}

/* Mengubah ukuran blok; bila p NULL hasilnya sama dengan alokasi baru. */
void *util_realloc(void *p, size_t n)
{
    void *r;

    if (alloc_should_fail()) {
        return NULL;
    }
    r = realloc(p, n);
    if (r == NULL) {
        return NULL;
    }
    if (p == NULL) {
        g_alloc_count++;
    }
    return r;
}

/* Melepas blok memori; p NULL aman dan tidak dihitung. */
void util_free(void *p)
{
    if (p == NULL) {
        return;
    }
    free(p);
    g_free_count++;
}

/* Selisih jumlah alokasi berhasil dan jumlah pelepasan. */
int util_alloc_balance(void)
{
    return (int)(g_alloc_count - g_free_count);
}

/* Mengatur injeksi kegagalan alokasi; nilai negatif mematikan injeksi. */
void util_fail_alloc_after(int k)
{
    g_fail_after = k;
}

/* Membaca seluruh isi berkas teks ke buffer yang diakhiri NUL. */
Status util_read_file(const char *path, char **out, size_t *len)
{
    FILE *f;
    long size;
    size_t got;
    char *buf;

    if (path == NULL || out == NULL || len == NULL) {
        return ST_BAD_INPUT;
    }
    f = fopen(path, "rb");
    if (f == NULL) {
        return ST_FILE_NOT_FOUND;
    }
    if (fseek(f, 0, SEEK_END) != 0) {
        fclose(f);
        return ST_FILE_NOT_FOUND;
    }
    size = ftell(f);
    if (size <= 0) {
        fclose(f);
        return ST_FILE_EMPTY;
    }
    rewind(f);
    buf = (char *)util_malloc((size_t)size + 1);
    if (buf == NULL) {
        fclose(f);
        return ST_NO_MEMORY;
    }
    got = fread(buf, 1, (size_t)size, f);
    fclose(f);
    buf[got] = '\0';
    *out = buf;
    *len = got;
    return ST_OK;
}

/* Menulis data mentah ke berkas teks. */
Status util_write_file(const char *path, const char *data, size_t len)
{
    FILE *f;
    size_t written;

    if (path == NULL || (data == NULL && len > 0)) {
        return ST_BAD_INPUT;
    }
    f = fopen(path, "wb");
    if (f == NULL) {
        return ST_FILE_WRITE;
    }
    written = (len > 0) ? fwrite(data, 1, len, f) : 0;
    if (written != len) {
        fclose(f);
        return ST_FILE_WRITE;
    }
    if (fclose(f) != 0) {
        return ST_FILE_WRITE;
    }
    return ST_OK;
}

/* Memulai kelompok uji baru dan mereset hitungan. */
void util_test_begin(const char *name)
{
    g_checks_total = 0;
    g_checks_failed = 0;
    printf("\n=== %s ===\n", (name != NULL) ? name : "Uji");
}

/* Mencatat satu pemeriksaan dan incremented jumlah kegagalan bila gagal. */
void util_check(int cond, const char *desc)
{
    g_checks_total++;
    if (cond) {
        printf("[PASS] %s\n", desc);
    } else {
        g_checks_failed++;
        printf("[FAIL] %s\n", desc);
    }
}

/* Memeriksa nilai floating point dengan toleransi eps. */
void util_check_double(double got, double want, double eps, const char *desc)
{
    double diff = got - want;
    if (diff < 0.0) {
        diff = -diff;
    }
    util_check(diff <= eps, desc);
}

/* Mencetak ringkasan kelompok uji dan mengembalikan jumlah kegagalan. */
int util_test_end(void)
{
    printf("Ringkasan: %d pemeriksaan, %d gagal\n", g_checks_total, g_checks_failed);
    return g_checks_failed;
}
