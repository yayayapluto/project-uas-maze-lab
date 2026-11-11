#include "maze_editor.h"

#include "maze_editor_cmd.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define EDITOR_MAX_ARGS 4

/* Memisahkan baris perintah menjadi token dan mengembalikan jumlah token. */
static int split_tokens(char *line, char *tokens[], int max)
{
    int count = 0;
    char *tok = strtok(line, " \t\r\n");

    while (tok != NULL && count < max) {
        tokens[count++] = tok;
        tok = strtok(NULL, " \t\r\n");
    }
    if (tok != NULL) {
        count++;
    }
    return count;
}

/* Menyatakan baris perintah berarti keluar dari editor. */
static int is_quit(const char *line)
{
    char buf[UTIL_MAX_LINE];
    char *tokens[2];
    int n;

    if (line == NULL) {
        return 0;
    }
    snprintf(buf, sizeof buf, "%s", line);
    n = split_tokens(buf, tokens, 2);
    if (n < 1) {
        return 0;
    }
    util_to_lower(tokens[0]);
    return strcmp(tokens[0], "quit") == 0;
}

/* Melepas isi stack undo. */
void maze_undo_stack_free(MazeUndoStack *st)
{
    if (st == NULL) {
        return;
    }
    util_free(st->items);
    st->items = NULL;
    st->count = 0;
    st->capacity = 0;
}

/* Menjalankan perintah yang tidak memerlukan labirin aktif. */
static Status exec_without_maze(Maze *slot, MazeUndoStack *st, char **tokens, int count,
                                 char *out, size_t out_size)
{
    int h;
    int w;

    if (strcmp(tokens[0], "new") == 0) {
        if (count != 3 || !maze_editor_parse_int(tokens[1], &h) || !maze_editor_parse_int(tokens[2], &w)) {
            maze_editor_say(out, out_size, "Argumen tidak valid. Gunakan help untuk daftar perintah.");
            return ST_BAD_INPUT;
        }
        return maze_editor_cmd_new(slot, st, h, w, out, out_size);
    }
    if (strcmp(tokens[0], "load") == 0) {
        if (count != 2) {
            maze_editor_say(out, out_size, "Argumen tidak valid. Gunakan help untuk daftar perintah.");
            return ST_BAD_INPUT;
        }
        return maze_editor_cmd_load(slot, st, tokens[1], out, out_size);
    }
    if (strcmp(tokens[0], "help") == 0) {
        maze_editor_cmd_help(out, out_size);
        return ST_OK;
    }
    maze_editor_say(out, out_size, "Keluar dari editor.");
    return ST_OK;
}

/* Menjalankan perintah yang mengubah isi labirin aktif. */
static Status exec_with_maze(Maze *slot, MazeUndoStack *st, char **tokens, int count,
                             char *out, size_t out_size)
{
    int r;
    int c;

    if (strcmp(tokens[0], "wall") == 0 || strcmp(tokens[0], "open") == 0 ||
        strcmp(tokens[0], "start") == 0 || strcmp(tokens[0], "end") == 0) {
        int kind;
        if (count != 3 || !maze_editor_parse_int(tokens[1], &r) || !maze_editor_parse_int(tokens[2], &c)) {
            maze_editor_say(out, out_size, "Argumen tidak valid. Gunakan help untuk daftar perintah.");
            return ST_BAD_INPUT;
        }
        if (strcmp(tokens[0], "wall") == 0) {
            kind = UNDO_WALL;
        } else if (strcmp(tokens[0], "open") == 0) {
            kind = UNDO_OPEN;
        } else if (strcmp(tokens[0], "start") == 0) {
            kind = UNDO_START;
        } else {
            kind = UNDO_END;
        }
        if (kind == UNDO_WALL || kind == UNDO_OPEN) {
            return maze_editor_cmd_cell(slot, st, r, c, kind, out, out_size);
        }
        return maze_editor_cmd_marker(slot, st, r, c, kind, out, out_size);
    }
    if (count != 1) {
        maze_editor_say(out, out_size, "Argumen tidak valid. Gunakan help untuk daftar perintah.");
        return ST_BAD_INPUT;
    }
    if (strcmp(tokens[0], "show") == 0) {
        maze_editor_cmd_show(slot);
        maze_editor_say(out, out_size, "Labirin ditampilkan.");
        return ST_OK;
    }
    if (strcmp(tokens[0], "check") == 0) {
        maze_editor_cmd_check(slot, out, out_size);
        return ST_OK;
    }
    if (strcmp(tokens[0], "undo") == 0) {
        return maze_editor_cmd_undo(slot, st, out, out_size);
    }
    if (strcmp(tokens[0], "help") == 0) {
        maze_editor_cmd_help(out, out_size);
        return ST_OK;
    }
    maze_editor_say(out, out_size, "Keluar dari editor.");
    return ST_OK;
}

/* Menjalankan satu baris perintah editor dan menulis pesan hasilnya. */
Status maze_editor_exec(Maze *slot, MazeUndoStack *st, const char *line, char *out, size_t out_size)
{
    char buf[UTIL_MAX_LINE];
    char *tokens[EDITOR_MAX_ARGS];
    int count;

    if (slot == NULL || st == NULL || line == NULL) {
        return ST_BAD_INPUT;
    }
    snprintf(buf, sizeof buf, "%s", line);
    count = split_tokens(buf, tokens, EDITOR_MAX_ARGS);
    if (count == 0) {
        maze_editor_say(out, out_size, "Perintah kosong. Gunakan help untuk daftar perintah.");
        return ST_BAD_INPUT;
    }
    util_to_lower(tokens[0]);
    if (slot->grid == NULL) {
        if (strcmp(tokens[0], "wall") == 0 || strcmp(tokens[0], "open") == 0 ||
            strcmp(tokens[0], "start") == 0 || strcmp(tokens[0], "end") == 0 ||
            strcmp(tokens[0], "show") == 0 || strcmp(tokens[0], "check") == 0 ||
            strcmp(tokens[0], "undo") == 0 || strcmp(tokens[0], "save") == 0) {
            maze_editor_say(out, out_size, "Belum ada labirin aktif. Gunakan new atau load.");
            return ST_BAD_INPUT;
        }
        return exec_without_maze(slot, st, tokens, count, out, out_size);
    }
    if (strcmp(tokens[0], "new") == 0) {
        return exec_without_maze(slot, st, tokens, count, out, out_size);
    }
    if (strcmp(tokens[0], "save") == 0 || strcmp(tokens[0], "load") == 0) {
        if (count != 2) {
            maze_editor_say(out, out_size, "Argumen tidak valid. Gunakan help untuk daftar perintah.");
            return ST_BAD_INPUT;
        }
        if (strcmp(tokens[0], "save") == 0) {
            return maze_editor_cmd_save(slot, tokens[1], out, out_size);
        }
        return maze_editor_cmd_load(slot, st, tokens[1], out, out_size);
    }
    return exec_with_maze(slot, st, tokens, count, out, out_size);
}

/* Menjalankan sesi editor sampai perintah quit diterima. */
Status maze_run_editor(Maze *slot)
{
    MazeUndoStack st;
    char line[UTIL_MAX_LINE];
    char out[UTIL_MAX_LINE];

    if (slot == NULL) {
        return ST_BAD_INPUT;
    }
    memset(&st, 0, sizeof st);
    maze_editor_say(out, sizeof out, "Editor labirin. Ketik help untuk daftar perintah, quit untuk keluar.");
    printf("%s\n", out);
    for (;;) {
        Status s = util_read_line("> ", line, sizeof line);
        if (s != ST_OK) {
            break;
        }
        if (is_quit(line)) {
            break;
        }
        maze_editor_exec(slot, &st, line, out, sizeof out);
        printf("%s\n", out);
    }
    maze_undo_stack_free(&st);
    return ST_OK;
}
