#ifndef MAZE_EDITOR_CMD_H
#define MAZE_EDITOR_CMD_H

#include "maze_editor.h"

/* Menulis pesan hasil perintah ke buffer keluaran dengan batas panjang. */
void maze_editor_say(char *out, size_t out_size, const char *text);

/* Mengurai bilangan bulat penuh; mengembalikan 0 bila bukan angka utuh. */
int maze_editor_parse_int(const char *s, int *out);

/* Menambahkan satu entri ke stack undo; diam-diam dilewati bila alokasi gagal. */
void maze_editor_undo_push(MazeUndoStack *st, int kind, int r, int c, char prev);

/* Menjalankan perintah new h w. */
Status maze_editor_cmd_new(Maze *slot, MazeUndoStack *st, int h, int w, char *out, size_t out_size);

/* Menjalankan perintah wall dan open pada satu sel. */
Status maze_editor_cmd_cell(Maze *m, MazeUndoStack *st, int r, int c, int kind,
                            char *out, size_t out_size);

/* Menjalankan perintah start dan end yang memindahkan penanda. */
Status maze_editor_cmd_marker(Maze *m, MazeUndoStack *st, int r, int c, int kind,
                               char *out, size_t out_size);

/* Membatalkan perintah ubah terakhir memakai stack undo. */
Status maze_editor_cmd_undo(Maze *m, MazeUndoStack *st, char *out, size_t out_size);

/* Menampilkan labirin dengan penomoran baris dan kolom. */
void maze_editor_cmd_show(const Maze *m);

/* Menjalankan pemeriksaan validasi dan pencarian jalur terpendek. */
void maze_editor_cmd_check(const Maze *m, char *out, size_t out_size);

/* Menyimpan labirin aktif ke berkas. */
Status maze_editor_cmd_save(const Maze *m, const char *name, char *out, size_t out_size);

/* Memuat labirin dari berkas tanpa merusak labirin aktif bila gagal. */
Status maze_editor_cmd_load(Maze *slot, MazeUndoStack *st, const char *name,
                             char *out, size_t out_size);

/* Menjalankan perintah help. */
void maze_editor_cmd_help(char *out, size_t out_size);

#endif
