# Prompt: Bangun Maze Lab (projek 04) di folder `04-maze-lab/`

## 1. Peran dan tujuan

Kamu adalah engineer C senior. Bangun projek **Maze Lab** secara utuh: enam modul, data uji, *test harness*, skrip build, dan README, semuanya di folder `04-maze-lab/`. Satu orang (kamu) mengerjakan seluruh peran. Pembagian lima anggota di PRD hanya konteks tim, tetapi batas modul dan berkas yang dinamai PRD tetap dipertahankan.

Kerjakan semua kebutuhan P0, lalu semua P1. Hasil akhir tidak boleh memuat *stub*, `TODO`, atau fungsi kosong.

## 2. Sumber kebenaran

Baca `PRD_04_Maze_Lab.md` sampai habis sebelum menulis kode. PRD itu menetapkan ID kebutuhan (MZ-01 dst.), format berkas (Bagian 7), contoh dan angka harapan (Bagian 8), algoritma (Bagian 9.5), kasus uji U01 sampai U20 (Bagian 12.2), dan kriteria selesai (Bagian 12.4).

- Soal **perilaku fitur**, PRD yang berlaku.
- Soal **struktur folder, build, dan isi `common/`**, prompt ini yang berlaku, karena PRD tidak menetapkannya.
- Jangan bertanya kecuali benar-benar terblokir. Untuk hal yang belum ditetapkan, ambil keputusan yang masuk akal dan catat di `README.md` pada bagian "Keputusan desain".

## 3. Lokasi dan struktur

Direktori kerja saat ini dianggap *root* proyek (`uas-suite/`). Program dijalankan dari root, sehingga path data berbentuk `04-maze-lab/data/kecil.txt`.

```
./
├── common/
│   ├── common.h
│   └── common.c
└── 04-maze-lab/
    ├── maze_lab.h / maze_lab.c            run_maze_lab, test_maze_lab, menu
    ├── maze_core.h / maze_core.c          tipe Maze, create/destroy/copy/print, load/save, validator
    ├── maze_catalog.h / maze_catalog.c
    ├── maze_gen.h / maze_gen.c
    ├── maze_solve.h / maze_solve.c
    ├── maze_editor.h / maze_editor.c
    ├── maze_bench.h / maze_bench.c
    ├── maze_main.c                        main() khusus standalone (bukan bagian suite)
    ├── data/                              lihat Bagian 9
    ├── README.md
    ├── Makefile
    └── build.bat
```

- `maze_main.c` hanya berisi `main`: argumen `--test` memanggil `test_maze_lab()` dan mengembalikan hasilnya sebagai kode keluar, tanpa argumen memanggil `run_maze_lab()`, argumen lain mencetak petunjuk pemakaian. Nanti saat digabung ke `suite`, berkas ini tidak ikut dikompilasi.
- `Makefile` di dalam `04-maze-lab/` menghasilkan executable `../maze_lab` (di root). Target: `all`, `test` (menjalankan `cd .. && ./maze_lab --test`), `asan` (build `-fsanitize=address,undefined` ke `../maze_lab_asan`), `clean`. `build.bat` setara untuk MinGW-w64 gcc (menghasilkan `..\maze_lab.exe`).
- Flag wajib: `-std=c99 -Wall -Wextra -Wpedantic -g`, tautan `-lm`. Nol *warning*.
- Include antarmodul memakai path relatif, misalnya `#include "../common/common.h"`, supaya tidak bergantung pada `-I`.

## 4. Pustaka bersama `common/`

Projek lain memakai `common/` yang sama. **Jika `common/common.h` dan `common.c` sudah ada, pakai apa adanya**: boleh menambah fungsi yang kurang, tetapi jangan mengubah tanda tangan yang sudah ada. Jika belum ada, buat sesuai kontrak berikut (PRD hanya menyebut nama fungsinya).

```c
typedef enum {
    ST_OK = 0, ST_FILE_NOT_FOUND, ST_FILE_EMPTY, ST_FILE_WRITE,
    ST_BAD_INPUT, ST_BAD_FORMAT, ST_OUT_OF_RANGE, ST_NO_MEMORY
} Status;

#define UTIL_MAX_LINE 256
#define UTIL_MAX_PATH 260

const char *util_status_message(Status s);
Status util_read_line(const char *prompt, char *buf, size_t size);
Status util_read_int(const char *prompt, int min, int max, int *out);
void   util_trim(char *s);
void   util_to_lower(char *s);
int    util_stricmp(const char *a, const char *b);
Status util_read_file(const char *path, char **out, size_t *len);
Status util_write_file(const char *path, const char *data, size_t len);
void  *util_malloc(size_t n);
void  *util_calloc(size_t n, size_t size);
void  *util_realloc(void *p, size_t n);
void   util_free(void *p);
int    util_alloc_balance(void);
void   util_fail_alloc_after(int k);
void   util_test_begin(const char *name);
void   util_check(int cond, const char *desc);
void   util_check_double(double got, double want, double eps, const char *desc);
int    util_test_end(void);
```

Perilaku yang harus dipenuhi:

- `util_status_message`: pesan bahasa Indonesia yang seragam untuk tiap `Status`, tidak pernah `NULL`.
- `util_read_line`: menampilkan `prompt` (boleh `NULL`), membaca satu baris dengan `fgets`, membuang `\n` dan `\r`. Baris yang lebih panjang dari buffer dipotong dan sisanya dibuang sampai akhir baris, sehingga tidak tertinggal di `stdin`. EOF menghasilkan `ST_BAD_INPUT` dan buffer kosong.
- `util_read_int`: mengulang sampai pengguna memberi bilangan bulat utuh dalam `[min, max]` (validasi dengan `strtol` dan `endptr`, spasi di tepi boleh, `12abc` ditolak). Menampilkan pesan saat salah. EOF menghasilkan `ST_BAD_INPUT`, tidak boleh berputar selamanya, dan pemanggil menganggap status bukan `ST_OK` sebagai tanda mengakhiri input.
- `util_read_file`: mode `"rb"`, mengembalikan buffer `util_malloc` yang diakhiri `'\0'` (`*len` = jumlah byte tanpa NUL). Berkas tidak ada: `ST_FILE_NOT_FOUND`. Berkas 0 byte: `ST_FILE_EMPTY` tanpa alokasi tersisa. Alokasi gagal: `ST_NO_MEMORY`.
- `util_write_file`: mode `"wb"`, gagal membuka atau menulis menghasilkan `ST_FILE_WRITE`.
- Pelacak alokasi: `util_alloc_balance()` = jumlah alokasi berhasil dikurangi jumlah pelepasan. `util_free(NULL)` aman dan tidak dihitung. `util_realloc(NULL, n)` setara alokasi. Jika `util_realloc` gagal, kembalikan `NULL` dan biarkan blok lama utuh. `util_fail_alloc_after(k)`: `k` alokasi berikutnya berhasil, semua alokasi sesudahnya mengembalikan `NULL` sampai dipanggil lagi dengan nilai negatif (`-1` mematikan injeksi).
- *Test harness*: `util_test_begin` mencetak judul dan mereset hitungan. `util_check` mencetak `[PASS]` atau `[FAIL]` beserta deskripsi dan menghitungnya. `util_check_double` lulus jika selisih mutlak tidak lebih dari `eps`. `util_test_end` mencetak ringkasan dan mengembalikan jumlah yang gagal (0 berarti semua lulus).
- Satu-satunya variabel file-scope yang diizinkan ada di `common.c` (pelacak alokasi dan hitungan harness), bertipe `static`. Kode projek tidak boleh punya variabel global.

## 5. Konvensi kode

- `snake_case`, fungsi di bawah 50 baris, tiap berkas `.c` di bawah 400 baris, tanpa `goto`, tanpa variabel global, parameter yang hanya dibaca diberi `const`.
- Semua alokasi lewat `util_malloc`/`util_calloc`/`util_realloc`. Hasil `NULL` dikembalikan sebagai `ST_NO_MEMORY`, semua alokasi yang sudah berhasil dilepas di jalur error (termasuk baris grid yang sudah teralokasi), hasil `realloc` ditampung di variabel sementara dulu, pointer diatur `NULL` setelah dilepas.
- Hanya C99 murni dan portabel (Windows MinGW dan Linux). Jangan pakai `strdup`, `strcasecmp`, `system()`, atau fungsi POSIX/Windows. Cetak `size_t` dengan `%lu` dan *cast* ke `unsigned long`. Hapus berkas sementara dengan `remove()`.
- Tidak ada `scanf` langsung. Semua input lewat `util_read_*`. `srand` hanya dipanggil di dalam generator.
- Rekursi (DFS, *flood fill*, backtracker) harus berframe kecil: tanpa array lokal besar, array besar dialokasikan di memori dinamis. DFS pada labirin terbuka 59 × 59 bisa sedalam 3.481 sel.
- Setiap akses tetangga sel lewat satu fungsi pemeriksa batas (misalnya `static int in_bounds(const Maze *m, int r, int c)`).
- **Komentar:** tiap fungsi diberi satu blok komentar bahasa Indonesia yang singkat berisi tujuan, parameter, nilai kembali, dan siapa yang melepas memori (diwajibkan PRD). Di luar itu, komentar hanya untuk logika yang tidak jelas dengan sendirinya. **Dilarang** komentar dekoratif atau pemisah seksi (misalnya `/* ==== */`, `// ----`, `/* ---- Bagian X ---- */`), juga kode yang dikomentari. Berlaku pula untuk Makefile dan `build.bat`.

## 6. Tipe dan prototipe

Tipe pada PRD Bagian 9.2 dipakai persis (`MazeCell`, `Maze`, `MazeSolveResult`, `MazeInfo`, `MazeCatalog`, `MazeBenchRow`). Nama fungsi publik pada Bagian 9.3 dipakai persis. Dokumen sumber yang memuat prototipe lengkap tidak tersedia, jadi gunakan rancangan berikut (kecuali dinyatakan lain, fungsi mengembalikan `Status` dan hasil lewat parameter keluaran). Boleh disesuaikan jika ada alasan kuat, asal konsisten dan dicatat di README.

```c
Status maze_create(Maze *m, int h, int w);
void   maze_destroy(Maze *m);
Status maze_copy(const Maze *src, Maze *dst);
void   maze_print(const Maze *m);
Status maze_load(const char *path, Maze *out);
Status maze_save(const char *path, const Maze *m);
Status maze_validate(const Maze *m, int *isolated, char *msg, size_t msg_size);
int    maze_count_dead_ends(const Maze *m);

Status maze_catalog_load(const char *path, MazeCatalog *cat, int *bad_lines);
Status maze_catalog_save(const char *path, const MazeCatalog *cat);
Status maze_catalog_add(MazeCatalog *cat, const MazeInfo *info);
void   maze_catalog_sort_by_size(MazeCatalog *cat);
void   maze_catalog_sort_by_difficulty(MazeCatalog *cat);
void   maze_catalog_sort_by_name(MazeCatalog *cat);
int    maze_catalog_find_name_seq(const MazeCatalog *cat, const char *name);
int    maze_catalog_find_name_binary(const MazeCatalog *cat, const char *name);
void   maze_catalog_free(MazeCatalog *cat);

Status maze_gen_backtracker(Maze *out, int h, int w, unsigned seed, int loops);
Status maze_gen_prim(Maze *out, int h, int w, unsigned seed, int loops);

Status maze_solve_dfs(const Maze *m, MazeSolveResult *res, MazeCell **path);
Status maze_solve_bfs(const Maze *m, MazeSolveResult *res, MazeCell **path);
Status maze_solve_wall(const Maze *m, MazeSolveResult *res, MazeCell **path);
void   maze_draw_path(Maze *m, const MazeCell *path, int length);

Status maze_run_editor(Maze *active);

Status maze_bench_run(int n, unsigned base_seed, int h, int w, int loops,
                      MazeBenchRow **rows, int *count);
void   maze_bench_summary(const MazeBenchRow *rows, int count, int n, int h, int w, int loops);
void   maze_bench_sort_by_visited(MazeBenchRow *rows, int count);
void   maze_bench_sort_by_length(MazeBenchRow *rows, int count);
int    maze_bench_find_seed(const MazeBenchRow *rows, int count, unsigned seed);
Status maze_bench_write_report(const char *path, const MazeBenchRow *rows, int count,
                               int n, int h, int w, int loops);
```

Catatan rancangan:

- `Maze` dipegang sebagai nilai (variabel lokal di `run_maze_lab`) dan dilewatkan lewat pointer. "Belum ada labirin aktif" berarti `grid == NULL`. `maze_destroy` menjadikan `grid = NULL` dan `h = w = 0` sehingga pemanggilan ganda aman. Fungsi yang gagal tidak boleh mengubah `Maze` keluarannya.
- Jalur hasil solver: array `MazeCell` berisi `length + 1` sel yang dialokasikan dinamis dan dilepas pemanggil dengan `util_free`. Jika `found = 0`, `length = 0` dan `*path = NULL`, sedangkan `visited` tetap terisi (dipakai benchmark).
- Pisahkan **pembaca perintah** dari **pelaksana perintah** di editor: tambahkan fungsi publik pelaksana satu baris perintah (misalnya `maze_editor_exec(Maze *m, MazeUndoStack *st, const char *line, char *out, size_t out_size)`) yang dipakai oleh `maze_run_editor` dan oleh `test_maze_lab()`, supaya U11 dan U12 bisa diuji tanpa `stdin`.
- Pisahkan perhitungan statistik benchmark dari pencetakan (misalnya `maze_bench_stats(rows, count, gen, solver, MazeBenchStat *out)`), supaya U17 dapat memeriksa angka dengan `util_check_double`.

## 7. Poin yang sering salah (patuhi)

**Core dan format**
- `maze_create` menolak `h`/`w` di luar 5 sampai 61 dengan `ST_OUT_OF_RANGE`, mengisi grid dengan `#`, dan jika alokasi baris ke-k gagal melepas baris 0 sampai k-1 serta array baris, lalu mengembalikan `ST_NO_MEMORY`.
- `maze_load` memuat ke `Maze` sementara dan baru menggantikan `out` setelah seluruh pemeriksaan lulus. Berkas rusak tidak boleh mengubah labirin aktif. Buang `\r` di akhir tiap baris. Baris kosong di akhir berkas diabaikan, tetapi baris tambahan yang tidak kosong berarti `ST_BAD_FORMAT`. Berkas hanya berisi spasi: `ST_BAD_FORMAT`.
- Tujuh kasus rusak pada Bagian 7.2 semuanya `ST_BAD_FORMAT`. Pemuatan hanya memeriksa format, bukan keterselesaian (`tanpa_jalan.txt` harus berhasil dimuat). `*` dimuat sebagai `.`.
- `maze_count_dead_ends`: sel jalan dengan tepat satu tetangga jalan; `S` dan `E` bukan kandidat tetapi tetap dihitung sebagai tetangga jalan. Untuk `kecil.txt` hasil harapannya 2, yaitu sel (4,1) dan (1,5). Konfirmasi dengan hitungan tangan.

**Katalog**
- Satu pembanding nama (`util_stricmp`) dipakai oleh Selection Sort nama **dan** Binary Search, agar urutan dan pencarian konsisten. Binary Search mengembalikan `-2` jika `sorted_by_name == 0`.
- `sorted_by_name = 1` hanya setelah urut nama. Urut ukuran, urut kesulitan, dan `maze_catalog_add` mengaturnya ke 0. Setelah `maze_catalog_load`, nilainya 0.
- Array dinamis mulai dari kapasitas kecil (misalnya 4) dan digandakan lewat `realloc`, sehingga `library.txt` yang berisi lebih dari 4 entri benar-benar menguji pertumbuhan.
- Baris rusak: jumlah kolom salah, angka tidak valid, `h`/`w` di luar 5 sampai 61, kesulitan di luar 1 sampai 5, nama kosong atau 32 karakter ke atas (jangan dipotong diam-diam). Baris rusak tidak menghentikan pemuatan dan jumlahnya dilaporkan lewat `bad_lines`.
- Menu katalog: lihat tabel, urut (ukuran, kesulitan, nama), cari (sequential dan binary; jika hasil `-2`, tawarkan urut nama dulu), tambah, buka (memuat berkas entri itu sebagai labirin aktif), serta simpan katalog ke path yang diketik. Katalog `04-maze-lab/data/library.txt` dimuat saat menu katalog pertama kali dibuka. Jika gagal, tampilkan pesan dan lanjut dengan katalog kosong.

**Generator**
- `h` dan `w` harus ganjil dan 5 sampai 61, selain itu `ST_OUT_OF_RANGE` (periksa sebelum alokasi apa pun). `S` di (1,1), `E` di (h-2, w-2).
- `srand(seed)` dipanggil di dalam generator, dan nol kali di tempat lain. Backtracker mengacak empat arah dengan Fisher-Yates. Prim memakai array frontier dinamis yang dilepas di akhir fungsi **termasuk** di jalur error.
- `loops`: kandidat adalah sel interior `#` dengan `(r + c)` ganjil (dinding di antara dua sel logis, bukan pilar). Acak kandidat, buka sebanyak `min(loops, jumlah kandidat)` tanpa error. Catat sebagai keputusan desain.
- Labirin sempurna (`loops = 0`): jumlah sel jalan = `2L - 1` dengan `L = ((h-1)/2) * ((w-1)/2)`, semua sel jalan terhubung (U04).
- Jangan menulis uji yang mengharapkan grid tertentu dari seed tertentu, karena `rand()` berbeda antarplatform. Uji hanya membandingkan dua pemanggilan dengan seed sama (U03).

**Solver**
- Urutan arah tetap untuk DFS dan BFS: atas, kanan, bawah, kiri. DFS rekursif dengan array `visited` (`char *`, `util_calloc`), jalur dibangun saat rekursi kembali, `visited` menghitung sel yang dimasuki. BFS: antrean array `int` berisi indeks `r * w + c` dan array `parent`, berhenti saat `E` dikeluarkan dari antrean, `visited` menghitung sel yang dikeluarkan (termasuk `E`), jalur direkonstruksi mundur dari `E`.
- *Wall follower*: arah awal timur, urutan percobaan belok kanan, lurus, belok kiri, putar balik, dan berhenti jika mencapai `E` atau setelah `4*h*w` langkah. Jalur yang disimpan adalah seluruh langkah (boleh memuat sel berulang), `length` jumlah langkah, `visited` jumlah sel berbeda yang pernah dilewati. Buffer jalur berkapasitas `4*h*w + 1`.
- Semua solver mengembalikan `found = 0` tanpa kebocoran pada labirin tanpa jalur. Panjang DFS hanya diuji `>=` panjang BFS.
- `maze_draw_path` menandai `*` kecuali pada `S` dan `E`. Menu solve selalu menggambar pada salinan (`maze_copy`), tidak pada labirin aktif.
- Menu solve menawarkan satu solver atau ketiganya. Untuk "ketiganya" cetak labirin berjalur beserta `found`/`length`/`visited` tiap solver, lalu satu tabel pembanding ringkas di akhir.

**Validator** (di `maze_core.c`)
- Periksa seluruh tepi `#`, tepat satu `S` dan satu `E`, lalu *flood fill* rekursif dari `S` (sel `.`, `S`, `E`, `*` dianggap jalan). `ST_OK` jika tepi, `S`, `E`, dan keterjangkauan `E` benar, selain itu `ST_BAD_FORMAT`. Sel terisolasi (jalan yang tak terjangkau) hanya dilaporkan: `msg` memuat kata "terisolasi" beserta jumlahnya dan status tidak berubah. Tulis `msg` dengan `snprintf` ber-batas.

**Editor**
- Perintah: `new h w`, `wall r c`, `open r c`, `start r c`, `end r c`, `show`, `check`, `undo`, `save nama`, `load nama`, `help`, `quit`. Tidak peka huruf besar/kecil. Koordinat dihitung dari nol, baris dulu lalu kolom.
- Parsing angka dengan `strtol` dan `endptr` (jangan `sscanf("%d")` mentah, karena `12abc` lolos). Argumen kurang, berlebih, atau bukan angka: `ST_BAD_INPUT` disertai petunjuk `help`. Baris kosong: pesan singkat, tidak crash.
- Editor boleh dimasuki tanpa labirin aktif. Perintah selain `new`, `load`, `help`, `quit` pada kondisi itu menampilkan pesan agar memakai `new` atau `load`.
- `new h w`: dimensi 5 sampai 61 (genap boleh), tepi `#`, bagian dalam `.`, `S` di (1,1), `E` di (h-2, w-2). `new` dan `load` membuat labirin baru lebih dulu dan baru menggantikan yang lama jika berhasil, lalu mengosongkan stack undo.
- `wall` dan `open` ditolak (`ST_OUT_OF_RANGE`, grid tidak berubah) jika sel di luar grid, di tepi, atau berupa `S`/`E`. Jika sel sudah berisi nilai tujuan, beri tahu dan jangan masukkan ke stack. `start` dan `end` ditolak jika tujuan di tepi, di luar grid, atau berupa titik lainnya (atau titik itu sendiri); sel lama menjadi `.`.
- Stack undo dinamis berisi `{jenis, r, c, nilai_sebelumnya}` sesuai Bagian 9.5 PRD: untuk `start`/`end`, `(r, c)` adalah posisi lama dan `nilai_sebelumnya` isi sel tujuan sebelum ditimpa; saat `undo`, posisi baru dibaca dari `m->start` atau `m->end`. `undo` pada stack kosong menampilkan pesan. Stack dilepas saat editor selesai.
- `show` mencetak penomoran kolom (dua baris digit puluhan dan satuan) dan nomor baris, supaya koordinat mudah ditentukan. `check` menjalankan `maze_validate` lalu BFS, dan melaporkan dapat diselesaikan atau tidak beserta panjang jalur, serta sel terisolasi jika ada.

**Benchmark**
- `maze_bench_run`: untuk tiap seed dari `base_seed` sampai `base_seed + n - 1`, bangkitkan satu labirin per generator (`"backtracker"`, `"prim"`), lalu jalankan `"dfs"`, `"bfs"`, `"wall"` dan simpan satu baris per kombinasi (total tepat `n * 2 * 3`). Lepas labirin dan jalur setiap iterasi. Jika alokasi gagal di tengah, lepas semua yang sudah terisi dan kembalikan `ST_NO_MEMORY` dengan `util_alloc_balance() == 0`.
- Ringkasan menghitung per kombinasi dengan memindai baris berdasarkan nama generator dan solver (bukan berdasarkan posisi), supaya tetap benar setelah baris diurutkan. Rata-rata, minimum, dan maksimum panjang hanya dari percobaan berhasil (cetak `-` jika tidak ada yang berhasil). Rata-rata `visited` dihitung dari seluruh percobaan. Gagal (%) = gagal / coba × 100.
- Histogram panjang jalur BFS: sepuluh kelas sama lebar antara minimum dan maksimum. Jaga agar tidak membagi nol (jika minimum sama dengan maksimum, cukup satu kelas) dan lewati histogram jika tidak ada BFS berhasil. Skala batang `#` jika hitungan terbesar terlalu panjang.
- Bubble Sort untuk urutan `visited`, Selection Sort untuk urutan panjang jalur, Sequential Search untuk seed. Jangan memakai `qsort`.
- Laporan berkas dibuat dari satu fungsi penyusun teks yang sama dengan tampilan layar, sehingga isinya identik (MZ-75). `n` pada menu 1 sampai 1000.
- Menu benchmark meminta `n`, `base_seed`, `h`, `w` (ganjil 5 sampai 61), dan `loops`, lalu menampilkan ringkasan dan histogram, kemudian menawarkan urut `visited`, urut panjang, cari seed, dan tulis laporan.

**Menu utama (`run_maze_lab`)**
- Ikuti Bagian 5 PRD. Labirin aktif, katalog, dan hasil benchmark adalah variabel lokal. Labirin hasil generate, buka dari katalog, dan editor menggantikan yang aktif setelah `maze_destroy` yang lama. Menu 3, 4 (untuk perintah selain `new` dan `load`), dan 6 memerlukan labirin aktif.
- Menu generate: algoritma (1 = backtracker, 2 = prim), `h`, `w` (ulang sampai ganjil dan 5 sampai 61, dengan pesan), seed (0 sampai 2147483647), `loops` (0 sampai 9999). Hasil ditampilkan dan menjadi labirin aktif.
- Jika `util_read_int` mengembalikan status bukan `ST_OK` (EOF), menu berakhir dengan bersih. Keluar dengan pilihan 0 melepaskan labirin aktif, katalog, dan semua alokasi lain.

## 8. Contoh yang dirancang untuk uji (salin persis dari PRD)

`kecil.txt` (6 × 7), `loop_putar.txt` (8 × 9), `tanpa_jalan.txt` (6 × 7), dan `terisolasi.txt` (5 × 7) isinya ada di Bagian 8.1 dan 8.2 PRD. Salin karakter demi karakter. Angka harapan: `kecil` BFS panjang 7; `loop_putar` BFS panjang 6 dan *wall follower* `found = 0`; `tanpa_jalan` ketiga solver `found = 0`; `terisolasi` validator melaporkan tepat satu sel terisolasi dengan `E` terjangkau dan BFS panjang 4. Jika kode memberi hasil berbeda, curigai kodemu dulu (arah awal timur dan urutan kanan, lurus, kiri, putar balik), baru simulasi ulang secara independen sebelum menyalahkan PRD.

## 9. Data uji

Isi `04-maze-lab/data/`:

- `kecil.txt`, `loop_putar.txt`, `tanpa_jalan.txt`, `terisolasi.txt` seperti Bagian 8.
- Tujuh berkas `rusak_*.txt` pada Bagian 7.2 PRD. Tiap berkas harus melanggar **hanya** aturan yang disebut namanya (misalnya `rusak_tepi.txt` punya dimensi benar, satu `S`, satu `E`, karakter sah, tetapi ada sel tepi bukan `#`), supaya uji benar-benar memeriksa jalur kode yang dimaksud.
- `katalog_rusak.txt`: campuran beberapa baris sah dan baris rusak dari tiap jenis (kolom salah, angka tidak valid, `h`/`w` di luar rentang, kesulitan di luar 1 sampai 5, nama terlalu panjang), dengan jumlah baris rusak yang diketahui persis dan dipakai di U15.
- `library.txt`: minimal sepuluh entri dengan ukuran, kesulitan (1 sampai 5), dan nama yang bervariasi (campur huruf besar dan kecil, urutan awal sengaja tidak terurut). Setiap entri menunjuk berkas labirin yang benar-benar ada di `data/`. Selain empat labirin di atas, buat enam atau lebih labirin lain (berbagai ukuran, sebagian dengan `loops > 0`) memakai generator buatanmu dengan seed tetap lewat program sementara yang **tidak** disertakan di hasil akhir, lalu bekukan sebagai berkas. `h`, `w`, dan `best_steps` di katalog harus sama dengan isi berkas (`best_steps` = panjang BFS, atau `-1` jika tidak ada jalur). Tambahkan di `test_maze_lab()` satu uji yang memuat `library.txt` dan memverifikasi hal itu untuk setiap entri.
- Berkas dengan akhir baris CRLF tidak wajib, tetapi `maze_load` harus tahan terhadapnya (uji dengan berkas sementara).

## 10. Pengujian

`test_maze_lab()` memuat seluruh U01 sampai U20 sebagai `util_check`, ditambah uji tambahan yang dianjurkan PRD (ukuran generator genap, 3, dan 63; `loops` melebihi kandidat; `maze_draw_path` tidak menimpa `S` dan `E`; solver pada labirin 61 × 61; `maze_destroy` dua kali) dan uji verifikasi `library.txt`. Fungsi ini mengembalikan 0 jika semua lulus. Uji kegagalan alokasi memakai `util_fail_alloc_after(k)` untuk beberapa `k` pada `maze_create` (U19), generator Prim, dan `maze_bench_run`, lalu mengembalikannya ke `-1`. Akhiri dengan `util_alloc_balance() == 0`. Berkas sementara (hasil simpan, laporan) ditulis ke `04-maze-lab/data/` dengan awalan `tmp_` dan dihapus dengan `remove()` di akhir.

Jika sebuah uji gagal, periksa kode lebih dulu. Ubah nilai harapan hanya jika hitungan independen membuktikan nilai itu salah.

## 11. Urutan kerja

1. Baca PRD sampai habis.
2. **Fondasi:** `common/`, `Maze`, `maze_create`, `maze_destroy`, `maze_copy`, `maze_print`, `maze_load`, `maze_save`, kerangka menu, Makefile dan `build.bat`, empat berkas contoh. Pastikan labirin bisa dimuat dan ditampilkan, build bersih tanpa *warning*.
3. **Modul:** generator (backtracker dulu, lalu Prim), solver, validator, editor, katalog, benchmark. Tiap modul disertai ujinya di `test_maze_lab()`.
4. **Integrasi:** hubungkan semua modul ke menu, lengkapi `data/`, pastikan U01 sampai U20 hijau.
5. **Verifikasi (jalankan benar-benar, jangan hanya mengklaim):**
   - `make` dengan tambahan `-Werror` berhasil tanpa satu pun *warning*.
   - `./maze_lab --test` selesai dengan kode keluar 0 dan `util_alloc_balance() == 0`.
   - Build ASan (`make asan`) lalu jalankan `--test` dan tiga skenario menu terpipa di Bagian 12.3 PRD (sesuaikan masukannya untuk executable standalone; dokumentasikan urutan masukan di README). Tidak boleh ada galat ASan, kebocoran, atau stack overflow, termasuk solver pada 61 × 61 dan benchmark n = 1000 pada 61 × 61.
   - Menu dan editor tidak crash pada EOF di tengah masukan, berkas rusak, perintah salah (`wall`, `wall a b`, `fly 1 1`, baris kosong), koordinat di tepi dan di luar grid.
   - Cek ukuran: tidak ada fungsi 50 baris atau lebih, tidak ada `.c` 400 baris atau lebih, tidak ada `goto`, tidak ada variabel global di kode projek, tidak ada komentar dekoratif, `srand` hanya ada di `maze_gen.c`. Jika `maze_core.c` atau `maze_solve.c` melewati 400 baris, pecah ke berkas pembantu (misalnya `maze_validate.c`) dan catat di README.
6. **README.md** projek: tujuan, cara build dan menjalankan (termasuk dari root `suite` kelak), format berkas (labirin dan katalog), daftar fungsi publik, daftar uji (U01 sampai U20 plus tambahan), skenario demonstrasi Bagian 15 PRD, keputusan desain (semua yang bertanda **[Keputusan]** di PRD ditambah keputusanmu sendiri), dan dua pertanyaan terbuka untuk dosen di Bagian 16.2.

## 12. Laporan akhir

Setelah semua selesai, tampilkan ringkasan singkat: pohon berkas yang dibuat, keluaran `--test` (jumlah lulus/gagal), hasil ASan, waktu benchmark n = 1000 pada 61 × 61, kasus tepi yang dicoba, serta daftar keputusan desain atau penyimpangan dari prompt ini beserta alasannya.
