#ifndef MAZE_EDITOR_CMD_H
#define MAZE_EDITOR_CMD_H

#include "maze_editor.h"

/* Menulis pesan hasil perintah ke buffer keluaran dengan batas panjang. */
void maze_editor_say(char *out, size_t out_size, const char *text);

/* Mengurai bilangan bulat penuh; mengembalikan 0 bila bukan angka utuh. */
int maze_editor_parse_int(const char *s, int *out);

/* Push satu entri ke stack undo; kalau alokasi gagal, entri dilewati. */
void maze_editor_undo_push(MazeUndoStack *st, int kind, int r, int c, char prev);

/* Menjalankan perintah new h w. */
Status maze_editor_cmd_new(Maze *slot, MazeUndoStack *st, int h, int w, char *out, size_t out_size);

/* Menjalankan perintah wall dan open pada satu sel. */
Status maze_editor_cmd_cell(Maze *m, MazeUndoStack *st, int r, int c, int kind,
                            char *out, size_t out_size);

/* Jalankan perintah start dan end untuk memindahkan penanda. */
Status maze_editor_cmd_marker(Maze *m, MazeUndoStack *st, int r, int c, int kind,
                               char *out, size_t out_size);

/* Batalkan perubahan terakhir memakai stack undo. */
Status maze_editor_cmd_undo(Maze *m, MazeUndoStack *st, char *out, size_t out_size);

/* Cetak labirin dengan nomor baris dan kolom. */
void maze_editor_cmd_show(const Maze *m);

/* Jalankan validasi, lalu pencarian jalur terpendek. */
void maze_editor_cmd_check(const Maze *m, char *out, size_t out_size);

/* Simpan labirin aktif ke file. */
Status maze_editor_cmd_save(const Maze *m, const char *name, char *out, size_t out_size);

/* Muat labirin dari file tanpa merusak labirin aktif kalau gagal. */
Status maze_editor_cmd_load(Maze *slot, MazeUndoStack *st, const char *name,
                             char *out, size_t out_size);

/* Menjalankan perintah help. */
void maze_editor_cmd_help(char *out, size_t out_size);

#endif
