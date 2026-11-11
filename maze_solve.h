#ifndef MAZE_SOLVE_H
#define MAZE_SOLVE_H

#include "maze_core.h"

/* Mencari jalur dengan DFS rekursif; jalur dilepas pemanggil dengan util_free. */
Status maze_solve_dfs(const Maze *m, MazeSolveResult *res, MazeCell **path);

/* Mencari jalur terpendek dengan BFS; jalur dilepas pemanggil dengan util_free. */
Status maze_solve_bfs(const Maze *m, MazeSolveResult *res, MazeCell **path);

/* Mencari jalur dengan wall follower; jalur dilepas pemanggil dengan util_free. */
Status maze_solve_wall(const Maze *m, MazeSolveResult *res, MazeCell **path);

/* Menandai jalur dengan karakter '*' tanpa menimpa S dan E. */
void maze_draw_path(Maze *m, const MazeCell *path, int length);

#endif
