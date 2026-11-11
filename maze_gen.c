#include "maze_gen.h"

#include <stdlib.h>
#include <string.h>

static const int GEN_DR[4] = { -1, 0, 1, 0 };
static const int GEN_DC[4] = { 0, 1, 0, -1 };

/* Memeriksa ukuran generator: ganjil dan berada pada rentang 5 sampai 61. */
static Status check_dims(int h, int w)
{
    if (h < 5 || h > 61 || w < 5 || w > 61) {
        return ST_OUT_OF_RANGE;
    }
    if (h % 2 == 0 || w % 2 == 0) {
        return ST_OUT_OF_RANGE;
    }
    return ST_OK;
}

/* Memperbesar array kandidat dinding; mengembalikan NULL bila alokasi gagal. */
static int *grow_candidates(int **cand, int *cap)
{
    int *tmp = (int *)util_realloc(*cand, (size_t)(*cap) * 2 * sizeof(int));

    if (tmp == NULL) {
        return NULL;
    }
    *cand = tmp;
    *cap = *cap * 2;
    return tmp;
}

/* Mengumpulkan dinding interior yang diapit dua sel jalan sebagai kandidat loops. */
static Status collect_candidates(const Maze *m, int **cand_out, int *count_out)
{
    int *cand = (int *)util_malloc(8 * sizeof(int));
    int cap = 8;
    int count = 0;
    int r;
    int c;

    if (cand == NULL) {
        return ST_NO_MEMORY;
    }
    for (r = 1; r < m->h - 1; r++) {
        for (c = 1; c < m->w - 1; c++) {
            int usable = 0;
            if (m->grid[r][c] == '#' && ((r + c) % 2) == 1) {
                if (r % 2 == 0) {
                    usable = (r - 1 >= 1 && r + 1 <= m->h - 2 &&
                              m->grid[r - 1][c] != '#' && m->grid[r + 1][c] != '#');
                } else {
                    usable = (c - 1 >= 1 && c + 1 <= m->w - 2 &&
                              m->grid[r][c - 1] != '#' && m->grid[r][c + 1] != '#');
                }
            }
            if (usable) {
                if (count == cap) {
                    int *tmp = grow_candidates(&cand, &cap);
                    if (tmp == NULL) {
                        util_free(cand);
                        return ST_NO_MEMORY;
                    }
                }
                cand[count++] = r * m->w + c;
            }
        }
    }
    *cand_out = cand;
    *count_out = count;
    return ST_OK;
}

/* Membuka sejumlah dinding kandidat secara acak sehingga terbentuk jalur alternatif. */
static Status open_loops(Maze *m, int loops)
{
    int *cand = NULL;
    int count = 0;
    int take;
    int i;
    Status st;

    if (loops <= 0) {
        return ST_OK;
    }
    st = collect_candidates(m, &cand, &count);
    if (st != ST_OK) {
        return st;
    }
    take = (loops < count) ? loops : count;
    for (i = 0; i < take; i++) {
        int j = i + rand() % (count - i);
        int tmp = cand[i];
        cand[i] = cand[j];
        cand[j] = tmp;
        m->grid[cand[i] / m->w][cand[i] % m->w] = '.';
    }
    util_free(cand);
    return ST_OK;
}

/* Menandai titik awal dan titik tujuan pada sel logis sudut. */
static void place_markers(Maze *m)
{
    m->grid[1][1] = 'S';
    m->grid[m->h - 2][m->w - 2] = 'E';
    m->start.r = 1;
    m->start.c = 1;
    m->end.r = m->h - 2;
    m->end.c = m->w - 2;
}

/* Mengacak urutan empat arah dengan Fisher-Yates. */
static void shuffle_dirs(int order[4])
{
    int i;

    order[0] = 0;
    order[1] = 1;
    order[2] = 2;
    order[3] = 3;
    for (i = 3; i > 0; i--) {
        int j = rand() % (i + 1);
        int tmp = order[i];
        order[i] = order[j];
        order[j] = tmp;
    }
}

/* Menggali sel logis dan seluruh dinding di thereof secara rekursif. */
static void bt_carve(Maze *m, char *vis, int r, int c)
{
    int order[4];
    int i;

    m->grid[r][c] = '.';
    vis[r * m->w + c] = 1;
    shuffle_dirs(order);
    for (i = 0; i < 4; i++) {
        int d = order[i];
        int nr = r + 2 * GEN_DR[d];
        int nc = c + 2 * GEN_DC[d];
        if (nr < 1 || nr > m->h - 2 || nc < 1 || nc > m->w - 2) {
            continue;
        }
        if (vis[nr * m->w + nc]) {
            continue;
        }
        m->grid[r + GEN_DR[d]][c + GEN_DC[d]] = '.';
        bt_carve(m, vis, nr, nc);
    }
}

/* Menjalankan recursive backtracker pada labirin sementara. */
static Status build_backtracker(Maze *m, unsigned seed)
{
    char *vis = (char *)util_calloc((size_t)(m->h * m->w), sizeof(char));
    Status st;

    if (vis == NULL) {
        return ST_NO_MEMORY;
    }
    srand(seed);
    bt_carve(m, vis, 1, 1);
    util_free(vis);
    st = ST_OK;
    return st;
}

/* Menambahkan dinding frontier yang bertetangga dengan sel logis baru. */
static Status prim_push_neighbours(Maze *m, int **front, int *count, int *cap, int r, int c)
{
    int d;

    for (d = 0; d < 4; d++) {
        int nr = r + GEN_DR[d];
        int nc = c + GEN_DC[d];
        if (nr < 1 || nr > m->h - 2 || nc < 1 || nc > m->w - 2) {
            continue;
        }
        if (m->grid[nr][nc] != '#') {
            continue;
        }
        if (*count == *cap) {
            int *tmp = (int *)util_realloc(*front, (size_t)(*cap) * 2 * sizeof(int));
            if (tmp == NULL) {
                return ST_NO_MEMORY;
            }
            *front = tmp;
            *cap = *cap * 2;
        }
        (*front)[(*count)++] = nr * m->w + nc;
    }
    return ST_OK;
}

/* Menjalankan randomized Prim pada labirin sementara. */
static Status build_prim(Maze *m, unsigned seed)
{
    char *vis = (char *)util_calloc((size_t)(m->h * m->w), sizeof(char));
    int *front = NULL;
    int count = 0;
    int cap = 0;
    Status st = ST_OK;

    if (vis == NULL) {
        return ST_NO_MEMORY;
    }
    srand(seed);
    vis[m->w + 1] = 1;
    m->grid[1][1] = '.';
    st = prim_push_neighbours(m, &front, &count, &cap, 1, 1);
    while (st == ST_OK && count > 0) {
        int pick = rand() % count;
        int cell = front[pick];
        int r = cell / m->w;
        int c = cell % m->w;
        int a;
        int b;
        int target;
        front[pick] = front[count - 1];
        count--;
        if (m->grid[r][c] != '#') {
            continue;
        }
        if (r % 2 == 0) {
            a = (r - 1) * m->w + c;
            b = (r + 1) * m->w + c;
        } else {
            a = r * m->w + (c - 1);
            b = r * m->w + (c + 1);
        }
        if (vis[a] == vis[b]) {
            continue;
        }
        target = vis[a] ? b : a;
        m->grid[r][c] = '.';
        vis[target] = 1;
        m->grid[target / m->w][target % m->w] = '.';
        st = prim_push_neighbours(m, &front, &count, &cap, target / m->w, target % m->w);
    }
    util_free(front);
    util_free(vis);
    return st;
}

/* Membangkitkan labirin dengan algoritma yang dipilih dan memindahkannya ke out. */
static Status generate(Maze *out, int h, int w, unsigned seed, int loops, int use_prim)
{
    Maze tmp;
    Status st;

    memset(&tmp, 0, sizeof tmp);
    st = check_dims(h, w);
    if (st != ST_OK) {
        return st;
    }
    st = maze_create(&tmp, h, w);
    if (st != ST_OK) {
        return st;
    }
    st = use_prim ? build_prim(&tmp, seed) : build_backtracker(&tmp, seed);
    if (st != ST_OK) {
        maze_destroy(&tmp);
        return st;
    }
    place_markers(&tmp);
    st = open_loops(&tmp, loops);
    if (st != ST_OK) {
        maze_destroy(&tmp);
        return st;
    }
    maze_destroy(out);
    *out = tmp;
    return ST_OK;
}

/* Membangkitkan labirin sempurna dengan recursive backtracker. */
Status maze_gen_backtracker(Maze *out, int h, int w, unsigned seed, int loops)
{
    if (out == NULL) {
        return ST_BAD_INPUT;
    }
    return generate(out, h, w, seed, loops, 0);
}

/* Membangkitkan labirin sempurna dengan randomized Prim. */
Status maze_gen_prim(Maze *out, int h, int w, unsigned seed, int loops)
{
    if (out == NULL) {
        return ST_BAD_INPUT;
    }
    return generate(out, h, w, seed, loops, 1);
}
