#ifndef MAZE_SOLVE_H
#define MAZE_SOLVE_H

#include "../maze_core/maze_core.h"

/* Cari jalur dengan DFS rekursif; pemanggil yang melepas jalurnya dengan util_free. */
Status maze_solve_dfs(const Maze *m, MazeSolveResult *res, MazeCell **path);

/* Cari jalur terpendek dengan BFS; pemanggil yang melepas jalurnya dengan util_free. */
Status maze_solve_bfs(const Maze *m, MazeSolveResult *res, MazeCell **path);

/* Cari jalur dengan wall follower; pemanggil yang melepas jalurnya dengan util_free. */
Status maze_solve_wall(const Maze *m, MazeSolveResult *res, MazeCell **path);

/* Menandai jalur dengan karakter '*' tanpa menimpa S dan E. */
void maze_draw_path(Maze *m, const MazeCell *path, int length);

#endif
