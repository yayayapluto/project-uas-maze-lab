#ifndef UI_H
#define UI_H

#include "../common/common.h"

/* Jenis warna untuk badge dan aksen tabel. */
typedef enum {
    UI_INFO,
    UI_TEXT,
    UI_OK,
    UI_WARN,
    UI_ERR,
    UI_MUTED
} UiKind;

/* Aktifkan dukungan ANSI true color dan UTF-8 pada terminal Windows.
 * Di platform lain tidak melakukan apa pun; aman dipanggil berkali-kali. */
void ui_init(void);

/* Cetak panel border ASCII (+, -, |) dengan judul di sisi atas dan
 * daftar baris teks di dalamnya. Konten dipotong bila lebih panjang dari
 * lebar panel. Tanpa warna saat stdout bukan terminal. */
void ui_panel(const char *title, const char *const *lines, int n_lines);

/* Cetak tabel berkepala berwarna. cells adalah array datar sepanjang
 * n_rows * n_cols (indeks baris i, kolom j = i * n_cols + j); widths adalah
 * lebar tiap kolom dalam jumlah karakter ASCII. */
void ui_table(const char *const *headers, const int *widths,
              const char *const *cells, int n_rows, int n_cols);

/* Cetak badge berwarna (latar solid, teks gelap) seperti pill modern. */
void ui_badge(const char *text, UiKind kind);

/* Badge hasil Status: ST_OK hijau, salah satu status masukan/format kuning,
 * kegagalan sistem merah, lalu diikuti pesan util_status_message. */
void ui_status(Status st);

#endif
