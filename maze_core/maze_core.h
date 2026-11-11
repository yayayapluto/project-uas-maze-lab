#ifndef MAZE_CORE_H
#define MAZE_CORE_H

#include "../common/common.h"

/* Satu sel labirin dengan koordinat baris dan kolom berbasis nol. */
typedef struct {
    int r;
    int c;
} MazeCell;

/* Labirin: grid karakter dua dimensi, titik awal, dan titik tujuan. */
typedef struct {
    int h;
    int w;
    char **grid;
    MazeCell start;
    MazeCell end;
} Maze;

/* Hasil satu solver: jalur ditemukan, panjang jalur, dan sel dikunjungi. */
typedef struct {
    int found;
    int length;
    int visited;
} MazeSolveResult;

/* Buat labirin berukuran h x w berisi dinding; pemanggil yang melepas via maze_destroy. */
Status maze_create(Maze *m, int h, int w);

/* Melepas seluruh isi labirin dan menjadikan labirin kosong; aman dipanggil dua kali. */
void maze_destroy(Maze *m);

/* Menyalin labirin ke labirin lain yang sudah diinisialisasi dengan {0}. */
Status maze_copy(const Maze *src, Maze *dst);

/* Menampilkan labirin ke layar. */
void maze_print(const Maze *m);

/* Muat labirin dari file; isi lama out dilepas hanya kalau pemuatan berhasil. */
Status maze_load(const char *path, Maze *out);

/* Simpan labirin ke file teks. */
Status maze_save(const char *path, const Maze *m);

/* Periksa labirin dan laporkan jumlah sel jalan terisolasi lewat msg. */
Status maze_validate(const Maze *m, int *isolated, char *msg, size_t msg_size);

/* Hitung sel jalan yang punya tepat satu tetangga jalan; S dan E dikecualikan. */
int maze_count_dead_ends(const Maze *m);

/* Menyatakan koordinat berada di dalam grid. */
int maze_in_bounds(const Maze *m, int r, int c);

#endif
