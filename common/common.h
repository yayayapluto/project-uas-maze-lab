#ifndef COMMON_H
#define COMMON_H

#include <stddef.h>

/* Status hasil operasi seragam untuk seluruh modul suite. */
typedef enum {
    ST_OK = 0,
    ST_FILE_NOT_FOUND,
    ST_FILE_EMPTY,
    ST_FILE_WRITE,
    ST_BAD_INPUT,
    ST_BAD_FORMAT,
    ST_OUT_OF_RANGE,
    ST_NO_MEMORY
} Status;

#define UTIL_MAX_LINE 256
#define UTIL_MAX_PATH 260

/* Pesan bahasa Indonesia untuk setiap status. Tidak pernah NULL. */
const char *util_status_message(Status s);

/* Baca satu baris ke buf; membuang newline dan sisa baris terlalu panjang. */
Status util_read_line(const char *prompt, char *buf, size_t size);

/* Baca bilangan bulat dalam [min, max]; mengulang sampai valid atau EOF. */
Status util_read_int(const char *prompt, int min, int max, int *out);

/* Buang spasi di tepi string. */
void util_trim(char *s);

/* Ubah huruf besar menjadi huruf kecil. */
void util_to_lower(char *s);

/* Bandingkan dua string tanpa membedakan huruf besar dan kecil. */
int util_stricmp(const char *a, const char *b);

/* Baca seluruh berkas teks; buffer dilepas pemanggil dengan util_free. */
Status util_read_file(const char *path, char **out, size_t *len);

/* Tulis data mentah ke berkas teks. */
Status util_write_file(const char *path, const char *data, size_t len);

/* Alokasi memori yang tercatat pelacak alokasi. */
void *util_malloc(size_t n);
void *util_calloc(size_t n, size_t size);
void *util_realloc(void *p, size_t n);
void util_free(void *p);

/* Selisih jumlah alokasi berhasil dan jumlah pelepasan. */
int util_alloc_balance(void);

/* Injeksi kegagalan alokasi: k alokasi berikutnya berhasil, sisanya gagal. */
void util_fail_alloc_after(int k);

/* Test harness sederhana untuk pengujian modul. */
void util_test_begin(const char *name);
void util_check(int cond, const char *desc);
void util_check_double(double got, double want, double eps, const char *desc);
int util_test_end(void);

#endif
