#ifndef MAZE_EDITOR_H
#define MAZE_EDITOR_H

#include "../maze_core/maze_core.h"

#define UNDO_WALL 0
#define UNDO_OPEN 1
#define UNDO_START 2
#define UNDO_END 3

/* Satu entri stack undo: jenis perubahan, posisi, dan isi sel sebelumnya. */
typedef struct {
    int kind;
    int r;
    int c;
    char prev;
} MazeUndoEntry;

/* Stack undo dinamis milik satu sesi editor. */
typedef struct {
    MazeUndoEntry *items;
    int count;
    int capacity;
} MazeUndoStack;

/* Menjalankan sesi editor; slot menunjuk ke variabel labirin aktif pemanggil. */
Status maze_run_editor(Maze *slot);

/* Menjalankan satu baris perintah editor dan menulis pesan hasilnya pada out. */
Status maze_editor_exec(Maze *slot, MazeUndoStack *st, const char *line, char *out, size_t out_size);

/* Melepas isi stack undo. */
void maze_undo_stack_free(MazeUndoStack *st);

#endif
