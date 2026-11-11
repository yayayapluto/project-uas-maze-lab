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

/* Kalau bernilai 1, alokasi berikutnya dibuat gagal (injeksi kegagalan). */
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

/* Pesan versi bahasa Indonesia untuk setiap status. */
const char *util_status_message(Status s)
{
    switch (s) {
    case ST_OK:            return "Berhasil.";
    case ST_FILE_NOT_FOUND: return "File tidak ditemukan.";
    case ST_FILE_EMPTY:    return "File kosong.";
    case ST_FILE_WRITE:    return "Gagal menulis file.";
    case ST_BAD_INPUT:     return "Input tidak valid.";
    case ST_BAD_FORMAT:    return "Format tidak sesuai.";
    case ST_OUT_OF_RANGE:  return "Nilai di luar rentang yang diperbolehkan.";
    case ST_NO_MEMORY:     return "Memori tidak cukup.";
    default:               return "Status tidak dikenal.";
    }
}

/* Buang spasi di awal dan akhir string. */
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

/* Ubah semua huruf besar menjadi huruf kecil, langsung di string asal. */
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

/* Bandingkan dua string tanpa peduli kapitalnya. */
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

/* Baca satu baris input; kalau lebih panjang dari buffer, sisanya dibuang supaya tidak ikut ke baris berikutnya. */
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

/* Baca satu bilangan bulat dalam rentang min-max; minta ulang sampai valid atau EOF. */
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
            printf("Input harus berupa angka.\n");
            continue;
        }
        if (value < (long)min || value > (long)max) {
            printf("Angkanya harus antara %d dan %d.\n", min, max);
            continue;
        }
        *out = (int)value;
        return ST_OK;
    }
}

/* Alokasi satu blok memori, dicatat di pelacak agar kebocoran bisa diperiksa. */
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

/* calloc versi sendiri: alokasi banyak elemen sekaligus diisi nol. */
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

/* realloc versi sendiri; kalau p NULL, perilakunya sama dengan alokasi baru. */
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

/* free versi sendiri; p NULL diabaikan dan tidak dihitung. */
void util_free(void *p)
{
    if (p == NULL) {
        return;
    }
    free(p);
    g_free_count++;
}

/* Selisih jumlah alokasi dan jumlah free; harus nol bila tidak ada kebocoran. */
int util_alloc_balance(void)
{
    return (int)(g_alloc_count - g_free_count);
}

/* Set injeksi kegagalan: k alokasi berikutnya berhasil, setelah itu semuanya gagal. Nilai negatif mematikan injeksi. */
void util_fail_alloc_after(int k)
{
    g_fail_after = k;
}

/* Baca seluruh isi file teks ke buffer yang diakhiri NUL. */
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

/* Tulis data mentah ke file teks. */
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

/* Mulai satu kelompok uji dan reset hitungan. */
void util_test_begin(const char *name)
{
    g_checks_total = 0;
    g_checks_failed = 0;
    printf("\n=== %s ===\n", (name != NULL) ? name : "Uji");
}

/* Catat satu pemeriksaan; kalau gagal, naikkan hitungan kegagalan. */
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

/* Cek dua nilai floating point dengan toleransi eps. */
void util_check_double(double got, double want, double eps, const char *desc)
{
    double diff = got - want;
    if (diff < 0.0) {
        diff = -diff;
    }
    util_check(diff <= eps, desc);
}

/* Cetak ringkasan kelompok uji dan kembalikan jumlah yang gagal. */
int util_test_end(void)
{
    printf("Ringkasan: %d pemeriksaan, %d gagal\n", g_checks_total, g_checks_failed);
    return g_checks_failed;
}
