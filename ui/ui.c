#include "ui.h"

#include <stdio.h>
#include <string.h>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0600
#endif
#ifndef CP_UTF8
#define CP_UTF8 65001
#endif
#include <windows.h>
#include <io.h>
#else
#include <unistd.h>
#endif

/* Palet warna 24-bit (dark theme): border biru tua, judul terang, teks
 * putih keunguan, dan aksen untuk badge. */
#define UI_RGB_BORDER 60, 76, 120
#define UI_RGB_TITLE 202, 211, 245
#define UI_RGB_TEXT 205, 214, 234
#define UI_RGB_MUTED 115, 125, 150
#define UI_RGB_INFO 129, 161, 255
#define UI_RGB_OK 158, 206, 106
#define UI_RGB_WARN 223, 177, 82
#define UI_RGB_ERR 246, 113, 118
#define UI_RGB_BADGE_FG 22, 26, 40

#define UI_RESET "\x1b[0m"

/* Periksa apakah stdout adalah terminal; warna hanya dipakai di situ. */
static int ui_use_color(void)
{
#ifdef _WIN32
    return _isatty(_fileno(stdout));
#else
    return isatty(fileno(stdout));
#endif
}

/* Kembalikan atribut teks ke bawaan terminal, hanya saat warna aktif. */
static void ui_reset(void)
{
    if (ui_use_color()) {
        printf(UI_RESET);
    }
}

/* Set warna foreground true color bila stdout adalah terminal. */
static void ui_set_fg(int r, int g, int b)
{
    if (ui_use_color()) {
        printf("\x1b[38;2;%d;%d;%dm", r, g, b);
    }
}

/* Set warna foreground dan background sekaligus untuk badge. */
static void ui_set_badge(int r, int g, int b)
{
    if (ui_use_color()) {
        printf("\x1b[38;2;%d;%d;%d;48;2;%d;%d;%dm", UI_RGB_BADGE_FG, r, g, b);
    }
}

/* Konversi UiKind menjadi warna aksen. */
static void ui_kind_fg(UiKind kind)
{
    switch (kind) {
    case UI_TEXT:  ui_set_fg(UI_RGB_TEXT);  break;
    case UI_OK:    ui_set_fg(UI_RGB_OK);    break;
    case UI_WARN:  ui_set_fg(UI_RGB_WARN);  break;
    case UI_ERR:   ui_set_fg(UI_RGB_ERR);   break;
    case UI_MUTED: ui_set_fg(UI_RGB_MUTED); break;
    default:       ui_set_fg(UI_RGB_INFO);  break;
    }
}

/* Aktifkan pemrosesan ANSI dan output UTF-8 pada konsol Windows. */
void ui_init(void)
{
#ifdef _WIN32
    HANDLE out = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode = 0;

    if (GetConsoleMode(out, &mode)) {
        SetConsoleMode(out, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
    }
    SetConsoleOutputCP(CP_UTF8);
#endif
}

/* Cetak karakter sama sebanyak n kali. */
static void ui_put_repeat(const char *glyph, int n)
{
    int i;

    for (i = 0; i < n; i++) {
        printf("%s", glyph);
    }
}

/* Cetak s lalu isi sisa lebar dengan spasi; konten dipotong bila lebih panjang. */
static void ui_put_pad(const char *s, int width)
{
    int len = (int)strlen(s);

    if (len > width) {
        len = width;
    }
    printf("%.*s", len, s);
    printf("%*s", width - len, "");
}

/* Hitung lebar dalam panel: baris terpanjang, minimal sepanjang judul. */
static int panel_inner(const char *title, const char *const *lines, int n_lines)
{
    int inner = (int)strlen(title) + 4;
    int i;

    for (i = 0; i < n_lines; i++) {
        int len = (int)strlen(lines[i]);
        if (len > inner) {
            inner = len;
        }
    }
    return inner;
}

/* Cetak sisi atas panel: +- Judul ----+. */
static void panel_top(const char *title, int span)
{
    int fill = span - (int)strlen(title) - 4;

    ui_set_fg(UI_RGB_BORDER);
    printf("+-");
    ui_reset();
    ui_set_fg(UI_RGB_TITLE);
    printf(" %s ", title);
    ui_reset();
    ui_set_fg(UI_RGB_BORDER);
    ui_put_repeat("-", fill);
    printf("+");
    ui_reset();
    printf("\n");
}

/* Cetak satu baris isi panel. */
static void panel_row(const char *text, int inner)
{
    ui_set_fg(UI_RGB_BORDER);
    printf("| ");
    ui_reset();
    ui_set_fg(UI_RGB_TEXT);
    ui_put_pad(text, inner);
    ui_reset();
    ui_set_fg(UI_RGB_BORDER);
    printf(" |");
    ui_reset();
    printf("\n");
}

/* Cetak panel border ASCII (+, -, |) dengan judul di sisi atas dan baris
 * teks di dalamnya. Konten dipotong bila lebih panjang dari lebar panel. */
void ui_panel(const char *title, const char *const *lines, int n_lines)
{
    int inner = panel_inner(title, lines, n_lines);
    int span = inner + 2;
    int i;

    panel_top(title, span);
    for (i = 0; i < n_lines; i++) {
        panel_row(lines[i], inner);
    }
    ui_set_fg(UI_RGB_BORDER);
    printf("+");
    ui_put_repeat("-", span);
    printf("+");
    ui_reset();
    printf("\n");
}

/* Cetak garis horizontal tabel dengan sudut dan penyambung sesuai jenis. */
static void table_border(const char *left, const char *mid, const char *right,
                         const int *widths, int n_cols)
{
    int j;

    ui_set_fg(UI_RGB_BORDER);
    printf("%s", left);
    for (j = 0; j < n_cols; j++) {
        ui_put_repeat("-", widths[j] + 2);
        if (j < n_cols - 1) {
            printf("%s", mid);
        }
    }
    printf("%s", right);
    ui_reset();
    printf("\n");
}

/* Cetak satu baris tabel dengan sel rata kiri. */
static void table_row(const char *const *cells, const int *widths, int n_cols, UiKind kind)
{
    int j;

    for (j = 0; j < n_cols; j++) {
        ui_set_fg(UI_RGB_BORDER);
        printf("| ");
        ui_kind_fg(kind);
        ui_put_pad(cells[j], widths[j]);
    }
    ui_set_fg(UI_RGB_BORDER);
    printf("|");
    ui_reset();
    printf("\n");
}

/* Cetak tabel berkepala: cells datar sepanjang n_rows * n_cols. */
void ui_table(const char *const *headers, const int *widths,
              const char *const *cells, int n_rows, int n_cols)
{
    int i;

    table_border("+", "+", "+", widths, n_cols);
    table_row(headers, widths, n_cols, UI_INFO);
    table_border("+", "+", "+", widths, n_cols);
    for (i = 0; i < n_rows; i++) {
        table_row(cells + (size_t)i * n_cols, widths, n_cols, UI_TEXT);
    }
    table_border("+", "+", "+", widths, n_cols);
}

/* Cetak badge berlatar warna solid dengan teks gelap. */
void ui_badge(const char *text, UiKind kind)
{
    switch (kind) {
    case UI_OK:    ui_set_badge(UI_RGB_OK);    break;
    case UI_WARN:  ui_set_badge(UI_RGB_WARN);  break;
    case UI_ERR:   ui_set_badge(UI_RGB_ERR);   break;
    case UI_MUTED: ui_set_badge(UI_RGB_MUTED); break;
    default:       ui_set_badge(UI_RGB_INFO);  break;
    }
    printf(" %s ", text);
    ui_reset();
}

/* Pilih label dan warna badge untuk satu Status. */
static void status_badge(Status st)
{
    switch (st) {
    case ST_OK:
        ui_badge("OK", UI_OK);
        break;
    case ST_FILE_WRITE:
    case ST_NO_MEMORY:
        ui_badge("FAIL", UI_ERR);
        break;
    default:
        ui_badge("WARN", UI_WARN);
        break;
    }
}

/* Tampilkan Status sebagai badge diikuti pesannya dalam satu baris. */
void ui_status(Status st)
{
    status_badge(st);
    printf(" %s\n", util_status_message(st));
}
