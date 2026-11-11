# Maze Lab

Program terminal berbahasa C99 untuk membuat, memeriksa, menyelesaikan, menyunting,
dan mengukur performa labirin ASCII. Semua modul ditulis modular dengan utilitas
bersama di `common/`, dan seluruh perilakunya mengikuti `PRD_04_Maze_Lab.md`.

## Cara Build dan Run

Toolchain: MinGW-w64 gcc (MSYS2 ucrt64) atau clang (MSYS2 clang64).

```sh
mingw32-make                          # build maze_lab
mingw32-make CFLAGS="-std=c99 -Wall -Wextra -Wpedantic -g -Werror"   # build ketat
mingw32-make test                     # build lalu ./maze_lab --test
mingw32-make asan                     # build ASan+UBSan (lihat catatan di bawah)
mingw32-make clean                    # hapus hasil build
```

Alternatif di Windows tanpa make:

```bat
build.bat          :: build maze_lab.exe
build.bat test     :: build lalu jalankan --test
build.bat asan     :: build ASan+UBSan
build.bat clean
```

Catatan ASan: gcc MinGW di mesin ini tidak menyertakan `libasan`, sehingga target
`asan` lewat gcc gagal saat link. Alternatif yang terverifikasi adalah clang dari
MSYS2 clang64:

```sh
PATH="/c/msys64/clang64/bin:$PATH"
clang -std=c99 -Wall -Wextra -Wpedantic -g -fsanitize=address,undefined \
      $(ls common/*.c maze_*/*.c maze_main.c) -o maze_lab_asan -lm
PATH="/c/msys64/clang64/bin:$PATH" ./maze_lab_asan --test
```

Menjalankan program:

```sh
./maze_lab          # menu utama
./maze_lab --test   # seluruh uji otomatis, exit 0 bila lolos
```

## Struktur Direktori

```
common/       utilitas lintas modul (io, string, alokasi terlacak, test harness)
maze_core/    struct Maze, buat/salin/muat/simpan/validasi
maze_gen/     generator backtracker dan Prim
maze_solve/   solver DFS, BFS, wall follower
maze_editor/  sesi editor + implementasi perintah
maze_catalog/ katalog labirin (urut, cari sequential/binary)
maze_bench/   benchmark ringkasan + laporan teks
maze_lab/     menu utama dan submenu katalog/benchmark
maze_test/    uji otomatis U01-U20 dan uji tambahan
ui/           panel, tabel, dan badge CLI bergaya modern (ANSI true color)
data/         berkas labirin, katalog contoh, dan berkas rusak untuk uji
maze_main.c   titik masuk program
```

## Format Berkas

**Labirin** (`.txt`): baris pertama `tinggi lebar`, diikuti `tinggi` baris isi.
Karakter yang sah: `#` (dinding), `.` (jalan), `S` (awal), `E` (tujuan). Tepi
labirin harus dinding, tepat satu `S`, tepat satu `E`, dan `E` harus terjangkau
dari `S`. Contoh di `data/kecil.txt`.

**Katalog** (`.txt`): satu entri per baris dengan kolom dipisah `;`:

```
nama;file;tinggi;lebar;difficulty;best_steps
kecil;data/kecil.txt;6;7;1;13
```

`difficulty` 1-5, `best_steps` -1 bila belum diketahui. Baris rusak dilaporkan
sebagai `bad_lines` dan tidak menghentikan pemuatan (uji memakai
`data/katalog_rusak.txt` dengan 5 baris rusak).

## Fungsi Publik Utama

- `common` — io (`util_read_line`, `util_read_int`, `util_read_file`,
  `util_write_file`), string (`util_trim`, `util_to_lower`, `util_stricmp`),
  alokasi terlacak (`util_malloc/calloc/realloc/free`, `util_alloc_balance`,
  `util_fail_alloc_after`), test harness (`util_test_begin/check/check_double/
  test_end`), dan `util_status_message` untuk pesan status.
- `maze_core` — `maze_create/destroy/copy/load/save/print`,
  `maze_validate`, `maze_count_dead_ends`, `maze_in_bounds`.
- `maze_gen` — `maze_gen_backtracker(out, h, w, seed, loops)` dan
  `maze_gen_prim` dengan signature sama.
- `maze_solve` — `maze_solve_dfs`, `maze_solve_bfs`, `maze_solve_wall`
  (mengisi `MazeSolveResult` dan jalur yang dilepas dengan `util_free`),
  `maze_draw_path`.
- `maze_editor` — `maze_run_editor(slot)` untuk sesi interaktif;
  `maze_editor_exec(slot, stack, line, out, out_size)` untuk satu baris
  perintah; perintah: `new h w`, `load nama`, `wall r c`, `open r c`,
  `start r c`, `end r c`, `show`, `check`, `undo`, `save nama`, `help`, `quit`.
  `save`/`load` otomatis menambah `.txt` dan menyimpan ke `data/`.
- `maze_catalog` — `maze_catalog_load/save/add`, tiga sort
  (`sort_by_size/difficulty/name`), pencarian `find_name_seq` dan
  `find_name_binary` (mengembalikan -2 bila belum terurut nama),
  `maze_catalog_free`.
- `maze_bench` — `maze_bench_run(n, base_seed, h, w, loops, &rows, &count)`,
  `maze_bench_summary`, sort `by_visited/by_length`, `find_seed`,
  `maze_bench_stats`, `maze_bench_write_report`.
- `ui` — `ui_init`, `ui_panel`, `ui_table`, `ui_badge`, `ui_status`.
- `maze_lab` — `run_maze_lab` (menu), `test_maze_lab` (`--test`),
  `maze_menu_catalog`, `maze_menu_bench`.

## Daftar Uji

Dijalankan dengan `./maze_lab --test` (semua kelompok harus PASS, exit 0):

- **U01** simpan lalu muat menghasilkan labirin identik.
- **U02** tujuh berkas rusak di `data/rusak_*.txt` ditolak dengan
  `ST_BAD_FORMAT`.
- **U03** seed sama menghasilkan grid identik untuk kedua algoritma.
- **U04** labirin sempurna punya `2L-1` sel jalan dan semuanya terhubung.
- **U05, U06** BFS menemukan jalur terpendek; DFS tidak pernah lebih pendek.
- **U07** ketiga solver gagal pada labirin tanpa jalur.
- **U08, U09** wall follower sukses di labirin sempurna, gagal di labirin putar.
- **U10** validator melaporkan sel terisolasi dan `E` yang tidak terjangkau.
- **U11, U12** perintah editor, undo, dan penolakan perintah salah.
- **U13** urutan katalog menurut ukuran, kesulitan, dan nama.
- **U14** binary search menolak katalog belum terurut (kode -2) lalu benar.
- **U15** baris rusak katalog dihitung tanpa menghentikan pemuatan.
- **U16** jumlah baris benchmark = `n x 2 generator x 3 solver`.
- **U17** statistik ringkasan cocok dengan hitungan manual.
- **U18** pencarian seed pada baris benchmark.
- **U19** kegagalan alokasi (`util_fail_alloc_after`) tidak menyisakan kebocoran.
- **U20** keseimbangan alokasi nol di akhir seluruh uji.
- **Tambahan** muat berkas berakhiran CRLF, editor tanpa labirin aktif,
  benchmark, verifikasi berkas `data/library.txt`.

## Skenario Demonstrasi (PRD 12.3)

1. Generate lalu solve: menu `2` (backtracker, 21x41, seed 7, loops 0), menu
   `3` pilih BFS (`found=1`), menu `6` simpan ke `data/tmp_demo1.txt`.
2. Katalog: menu `1`, urutkan nama, cari binary `Kecil`, buka entri pertama.
3. Editor: menu `4`, `new 11 21`, `wall 3 4`, `check`, `undo`, lalu coba
   perintah salah (`wall 0 5`, `fly 1 1`, `wall`) yang semuanya ditolak.
4. Benchmark: menu `5`, n=1000, 61x61; di mesin pengembangan selesai di bawah
   satu detik (build biasa).

## Keputusan Desain

- [Keputusan] Generator bekerja pada "sel logis": `S` di `(1,1)`, `E` di
  `(h-2, w-2)`, dan loop ditambahkan dengan membuka dinding interior yang
  diapit dua jalan, sehingga tepi selalu utuh sebagai dinding.
- [Keputusan] Solver dan validator memakai arah kunjungan tetap
  (atas, kanan, bawah, kiri) supaya hasilnya deterministik lintas run.
- [Keputusan] Wall follower dibatasi `4*h*w` langkah; melewati batas berarti
  gagal dengan `found=0` dan `*path=NULL`, bukan hang.
- [Keputusan] Katalog menandai `sorted_by_name=0` setiap kali isi berubah;
  binary search menolak dengan -2 alih-alih memberi hasil salah.
- [Keputusan] Editor memakai value semantics: `Maze *slot` menunjuk variabel
  milik pemanggil, "belum aktif" ditandai `slot->grid == NULL`.
- [Keputusan] Semua alokasi lewat util terlacak sehingga U19/U20 bisa
  membuktikan tidak ada kebocoran walau alokasi sengaja digagalkan.

Keputusan tambahan yang kami ambil sendiri:

- Berkas source dipisah per modul dalam folder masing-masing mengikuti pola
  proyek text-toolkit; `maze_validate.c`, `maze_editor_cmd.c`, dan
  `maze_bench_report.c` dipecah dari modul induknya agar tidak ada `.c` di
  atas 400 baris dan tidak ada fungsi di atas 50 baris.
- `maze_main.c` di root sebagai titik masuk kecil, memanggil `ui_init`
  (setlocale + aktivasi ANSI pada terminal Windows) lalu `run_maze_lab`.
- `save`/`load` editor menerima nama tanpa ekstensi dan otomatis menambah
  `.txt` di folder `data/`, supaya path yang diketik pengguna tetap pendek.
- Modul `ui/` hanya menyuntik kode ANSI saat stdout terminal (deteksi
  `isatty`); saat output dipipa ke berkas, warna otomatis dimatikan agar log
  tetap bersih.
- gcc MinGW di mesin ini tidak punya `libasan`, jadi verifikasi ASan memakai
  clang dari MSYS2 clang64; kedua build menghasilkan test yang sama.

## Pertanyaan Terbuka

- PRD 16.2 menanyakan: apakah `loops` perlu dibatasi selain jumlah kandidat
  dinding? Saat ini `loops` melebihi kandidat hanya membuka semuanya (uji
  tersendiri memastikan ini tidak error), tidak ada batas keras lain.
