#include "maze_solve.h"

#include <stdlib.h>
#include <string.h>

static const int SOLVE_DR[4] = { -1, 0, 1, 0 };
static const int SOLVE_DC[4] = { 0, 1, 0, -1 };

/* Mengisi hasil solver dengan nilai awal dan menandai jalur kosong. */
static void reset_result(MazeSolveResult *res, MazeCell **path)
{
    res->found = 0;
    res->length = 0;
    res->visited = 0;
    *path = NULL;
}

/* DFS rekursif: entering sel menandai kunjungan, jalur disusun saat rekursi kembali. */
static int dfs_go(const Maze *m, int r, int c, char *seen, MazeCell *stack, int *sp, int *visited)
{
    int d;

    if (!maze_in_bounds(m, r, c) || m->grid[r][c] == '#') {
        return 0;
    }
    if (seen[r * m->w + c]) {
        return 0;
    }
    seen[r * m->w + c] = 1;
    (*visited)++;
    stack[*sp].r = r;
    stack[*sp].c = c;
    (*sp)++;
    if (r == m->end.r && c == m->end.c) {
        return 1;
    }
    for (d = 0; d < 4; d++) {
        int nr = r + SOLVE_DR[d];
        int nc = c + SOLVE_DC[d];
        if (dfs_go(m, nr, nc, seen, stack, sp, visited)) {
            return 1;
        }
    }
    (*sp)--;
    return 0;
}

/* Menyalin stack DFS menjadi jalur keluaran; pemanggil melepas dengan util_free. */
static Status dfs_emit(const MazeCell *stack, int len, MazeCell **path)
{
    MazeCell *out = (MazeCell *)util_malloc((size_t)(len + 1) * sizeof(MazeCell));

    if (out == NULL) {
        return ST_NO_MEMORY;
    }
    memcpy(out, stack, (size_t)len * sizeof(MazeCell));
    *path = out;
    return ST_OK;
}

/* Mencari jalur dengan DFS rekursif dan mengisi hasil solver. */
Status maze_solve_dfs(const Maze *m, MazeSolveResult *res, MazeCell **path)
{
    char *seen;
    MazeCell *stack;
    int sp = 0;
    int visited = 0;
    Status st;

    if (m == NULL || res == NULL || path == NULL || m->grid == NULL) {
        return ST_BAD_INPUT;
    }
    reset_result(res, path);
    seen = (char *)util_calloc((size_t)(m->h * m->w), sizeof(char));
    stack = (MazeCell *)util_malloc((size_t)(m->h * m->w) * sizeof(MazeCell));
    if (seen == NULL || stack == NULL) {
        util_free(seen);
        util_free(stack);
        return ST_NO_MEMORY;
    }
    if (dfs_go(m, m->start.r, m->start.c, seen, stack, &sp, &visited)) {
        st = dfs_emit(stack, sp, path);
        if (st != ST_OK) {
            util_free(seen);
            util_free(stack);
            return st;
        }
        res->found = 1;
        res->length = sp - 1;
    }
    res->visited = visited;
    util_free(seen);
    util_free(stack);
    return ST_OK;
}

/* Merekonstruksi jalur dari E mundur mengikuti array parent. */
static Status bfs_rebuild(const Maze *m, const int *parent, MazeCell **path, int *length)
{
    int idx = m->end.r * m->w + m->end.c;
    int start = m->start.r * m->w + m->start.c;
    int count = 0;
    int i;
    MazeCell *out;

    while (idx != start && parent[idx] != -1) {
        count++;
        idx = parent[idx];
    }
    if (idx != start) {
        return ST_BAD_FORMAT;
    }
    out = (MazeCell *)util_malloc((size_t)(count + 1) * sizeof(MazeCell));
    if (out == NULL) {
        return ST_NO_MEMORY;
    }
    idx = m->end.r * m->w + m->end.c;
    for (i = count; i >= 0; i--) {
        out[i].r = idx / m->w;
        out[i].c = idx % m->w;
        if (idx == start) {
            break;
        }
        idx = parent[idx];
    }
    *path = out;
    *length = count;
    return ST_OK;
}

/* Mencari jalur terpendek dengan BFS dan mengisi hasil solver. */
Status maze_solve_bfs(const Maze *m, MazeSolveResult *res, MazeCell **path)
{
    int *queue;
    int *parent;
    int head = 0;
    int tail = 0;
    int visited = 0;
    int start;
    Status st;

    if (m == NULL || res == NULL || path == NULL || m->grid == NULL) {
        return ST_BAD_INPUT;
    }
    reset_result(res, path);
    queue = (int *)util_malloc((size_t)(m->h * m->w) * sizeof(int));
    parent = (int *)util_malloc((size_t)(m->h * m->w) * sizeof(int));
    if (queue == NULL || parent == NULL) {
        util_free(queue);
        util_free(parent);
        return ST_NO_MEMORY;
    }
    memset(parent, -1, (size_t)(m->h * m->w) * sizeof(int));
    start = m->start.r * m->w + m->start.c;
    parent[start] = start;
    queue[tail++] = start;
    while (head < tail) {
        int idx = queue[head++];
        int r = idx / m->w;
        int c = idx % m->w;
        int d;
        visited++;
        if (r == m->end.r && c == m->end.c) {
            break;
        }
        for (d = 0; d < 4; d++) {
            int nr = r + SOLVE_DR[d];
            int nc = c + SOLVE_DC[d];
            int nidx;
            if (!maze_in_bounds(m, nr, nc) || m->grid[nr][nc] == '#') {
                continue;
            }
            nidx = nr * m->w + nc;
            if (parent[nidx] != -1) {
                continue;
            }
            parent[nidx] = idx;
            queue[tail++] = nidx;
        }
    }
    if (parent[m->end.r * m->w + m->end.c] != -1) {
        int length = 0;
        st = bfs_rebuild(m, parent, path, &length);
        if (st == ST_OK) {
            res->found = 1;
            res->length = length;
        } else {
            util_free(queue);
            util_free(parent);
            return st;
        }
    }
    res->visited = visited;
    util_free(queue);
    util_free(parent);
    return ST_OK;
}

/* Wall follower: mencoba belok kanan, lurus, belok kiri, lalu putar balik. */
static int wall_choose_dir(const Maze *m, int r, int c, int dir)
{
    static const int order[4] = { 1, 0, 3, 2 };
    int i;

    for (i = 0; i < 4; i++) {
        int d = (dir + order[i]) % 4;
        int nr = r + SOLVE_DR[d];
        int nc = c + SOLVE_DC[d];
        if (maze_in_bounds(m, nr, nc) && m->grid[nr][nc] != '#') {
            return d;
        }
    }
    return -1;
}

/* Mengikuti dinding kanan sampai mencapai E atau melewati batas langkah. */
Status maze_solve_wall(const Maze *m, MazeSolveResult *res, MazeCell **path)
{
    int limit;
    char *seen;
    MazeCell *out;
    int r;
    int c;
    int dir = 1;
    int steps = 0;
    int visited = 0;

    if (m == NULL || res == NULL || path == NULL || m->grid == NULL) {
        return ST_BAD_INPUT;
    }
    reset_result(res, path);
    limit = 4 * m->h * m->w;
    seen = (char *)util_calloc((size_t)(m->h * m->w), sizeof(char));
    out = (MazeCell *)util_malloc((size_t)(limit + 1) * sizeof(MazeCell));
    if (seen == NULL || out == NULL) {
        util_free(seen);
        util_free(out);
        return ST_NO_MEMORY;
    }
    r = m->start.r;
    c = m->start.c;
    seen[r * m->w + c] = 1;
    visited = 1;
    out[0].r = r;
    out[0].c = c;
    while (steps < limit) {
        int d = wall_choose_dir(m, r, c, dir);
        if (d < 0) {
            break;
        }
        r += SOLVE_DR[d];
        c += SOLVE_DC[d];
        dir = d;
        steps++;
        out[steps].r = r;
        out[steps].c = c;
        if (!seen[r * m->w + c]) {
            seen[r * m->w + c] = 1;
            visited++;
        }
        if (r == m->end.r && c == m->end.c) {
            res->found = 1;
            break;
        }
    }
    res->length = steps;
    res->visited = visited;
    if (!res->found) {
        util_free(out);
        *path = NULL;
        res->length = 0;
    } else {
        *path = out;
    }
    util_free(seen);
    return ST_OK;
}

/* Menandai jalur dengan '*' tanpa menimpa titik awal dan titik tujuan. */
void maze_draw_path(Maze *m, const MazeCell *path, int length)
{
    int i;

    if (m == NULL || m->grid == NULL || path == NULL) {
        return;
    }
    for (i = 0; i <= length; i++) {
        int r = path[i].r;
        int c = path[i].c;
        if (!maze_in_bounds(m, r, c)) {
            continue;
        }
        if (m->grid[r][c] == 'S' || m->grid[r][c] == 'E') {
            continue;
        }
        m->grid[r][c] = '*';
    }
}
