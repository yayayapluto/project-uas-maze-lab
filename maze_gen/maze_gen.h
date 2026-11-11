#ifndef MAZE_GEN_H
#define MAZE_GEN_H

#include "../maze_core/maze_core.h"

/* Membangkitkan labirin sempurna dengan recursive backtracker; seed menentukan hasil. */
Status maze_gen_backtracker(Maze *out, int h, int w, unsigned seed, int loops);

/* Membangkitkan labirin sempurna dengan randomized Prim; seed menentukan hasil. */
Status maze_gen_prim(Maze *out, int h, int w, unsigned seed, int loops);

#endif
