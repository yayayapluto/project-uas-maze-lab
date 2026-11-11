# PRD Maze Lab

| | |
|---|---|
| **Produk** | Maze Lab (projek 04 dalam `uas-suite/`) |
| **Mata kuliah** | Dasar Pemrograman (INF124102), S1 Informatika UPNVJ, Ganjil 2026/2027 |
| **Versi dokumen** | 1.0, 11 November 2026 |
| **Folder** | `04-maze-lab/` |
| **Prefix fungsi publik** | `maze_` |
| **Pintu masuk** | `void run_maze_lab(void)` dan `int test_maze_lab(void)` |
| **Tim** | 5 orang |
| **Sumber spesifikasi** | Dokumen *Ide Projek UAS Daspro*, Bagian 0, 1.4, 2.0, 2.4, dan 2.5 |

Dokumen ini menguraikan kebutuhan Maze Lab sampai tingkat yang cukup untuk dikerjakan, diuji, dan didemonstrasikan. Nama fungsi, tipe, dan format berkas mengikuti dokumen sumber. Jika ada perbedaan, dokumen sumber yang berlaku. Keputusan yang diambil di sini karena sumber belum menetapkannya ditandai **[Keputusan]** dan dikumpulkan di Bagian 16.

---

## 1. Ringkasan

Maze Lab adalah satu "laboratorium" labirin berbasis terminal yang ditulis dalam C. Labirin disimpan sebagai berkas teks, diatur dalam katalog, dibangkitkan oleh dua algoritma generator, diselesaikan oleh tiga algoritma solver, disunting lewat perintah teks, dan diperiksa oleh validator. Modul benchmark menjalankan semua kombinasi generator dan solver pada banyak labirin, lalu membandingkan hasilnya dalam ringkasan dan laporan.

Program hanya memakai pustaka standar C99 dan menjadi projek keempat yang diluncurkan lewat satu menu utama (`./suite`).

## 2. Latar belakang dan tujuan

### 2.1 Latar belakang

Projek menyumbang 50% nilai akhir praktikum dan dinilai lewat penilaian projek, demonstrasi, dan tanya jawab (minggu 16). Indikator penilaiannya menuntut program modular yang memadukan struktur kontrol, fungsi, array dan string, pointer, struct, searching dan sorting, serta file teks dengan penanganan kesalahan.

Maze Lab dipilih karena demonstrasinya paling enak dilihat. Rekursi dan array dua dimensi dipakai secara alami, dan sorting serta searching mendapat tempat yang wajar di katalog dan benchmark. Kelemahannya ada pada beban kerja yang timpang: generator dan solver jauh lebih berat daripada modul lain. Pembagian tugas pada Bagian 15 sudah memperhitungkan hal ini.

### 2.2 Sasaran produk

| Kode | Sasaran | Ukuran keberhasilan |
|---|---|---|
| S1 | Seluruh fitur pada Bagian 6 berfungsi sungguhan, bukan *stub* | Semua kebutuhan P0 lulus kriteria penerimaan |
| S2 | Labirin dapat direproduksi | Seed yang sama menghasilkan grid identik untuk kedua generator |
| S3 | Program tidak crash pada input apa pun | Berkas rusak, perintah salah, dan koordinat di luar rentang menghasilkan pesan yang jelas |
| S4 | Tidak ada kebocoran memori, termasuk pada jalur gagal alokasi | `util_alloc_balance() == 0` di akhir tiap uji, termasuk uji dengan `util_fail_alloc_after(k)` |
| S5 | Perbandingan algoritma dapat dipertanggungjawabkan | Ringkasan benchmark cocok dengan hitungan manual pada kasus kecil |
| S6 | Setiap anggota dapat menjelaskan bagian yang dipegangnya | Tiap modul punya satu pemilik dan komentar fungsi dalam bahasa Indonesia |

### 2.3 Di luar cakupan

- Tampilan grafis, warna terminal, animasi, dan fungsi khusus sistem operasi (`system()`, `cls`).
- Labirin berukuran di luar 5 sampai 61 sel per sisi.
- Algoritma lain, misalnya A\*, Dijkstra, Kruskal, atau Wilson.
- Labirin tiga dimensi, bobot sel, atau lebih dari satu titik awal dan akhir.
- Berkas biner. Semua penyimpanan berupa berkas teks.

## 3. Pengguna dan skenario

| Pengguna | Kebutuhan | Skenario utama |
|---|---|---|
| Mahasiswa pengguna | Membuat, menyelesaikan, dan membandingkan labirin | Membangkitkan labirin 21×41 dengan seed tertentu, menyelesaikannya dengan tiga solver, lalu melihat perbedaan panjang jalur |
| Dosen atau asisten penguji | Menguji batas dan menanyakan logika algoritma | Memuat berkas tanpa `S`, mengetik `wall 0 0` di editor, atau meminta benchmark dengan n = 1 |
| Anggota tim lain | Memakai fungsi modul lain tanpa bentrok | Memanggil `maze_solve_bfs` dari modul benchmark tanpa membaca isinya |

## 4. Gambaran fitur

| Modul | File | Isi | Pemilik |
|---|---|---|---|
| Core dan katalog | `maze_core.c`, `maze_catalog.c` | Tipe `Maze`, muat dan simpan, katalog dengan sorting dan searching, *test harness* | Anggota 1 |
| Generator | `maze_gen.c` | *Recursive backtracker*, *randomized Prim*, jalur alternatif (`loops`) | Anggota 2 |
| Solver | `maze_solve.c` | DFS rekursif, BFS, *wall follower* | Anggota 3 |
| Editor dan validator | `maze_editor.c`, `maze_validate` | Editor perintah teks dengan `undo`, validasi dengan *flood fill* | Anggota 4 |
| Benchmark | `maze_bench.c` | Eksperimen banyak seed, ringkasan, histogram, laporan | Anggota 5 |

## 5. Alur pengguna

Menu Maze Lab:

```
=== MAZE LAB ===
1. Katalog (lihat, urutkan, cari, tambah, buka)
2. Generate labirin
3. Selesaikan labirin (DFS / BFS / wall follower)
4. Editor labirin
5. Benchmark algoritma
6. Simpan labirin aktif
0. Kembali
```

Aturan alur:

1. Labirin aktif disimpan sebagai variabel lokal di `run_maze_lab` dan dilewatkan lewat pointer. Pada awal program belum ada labirin aktif.
2. Menu 3, 4, dan 6 memerlukan labirin aktif. Menu 4 juga dapat membuat labirin baru lewat perintah `new` atau `load`.
3. Labirin hasil menu 2, hasil `buka` dari katalog, dan hasil editor menggantikan labirin aktif. Labirin lama dilepas dulu lewat `maze_destroy`.
4. Menu 3 menggambar jalur pada salinan labirin (`maze_copy`), sehingga labirin aktif tidak berubah.
5. Semua input lewat fungsi `util_read_*`. Pilihan di luar rentang atau berisi huruf diulang sampai valid.
6. Keluar dari menu (pilihan 0) melepaskan labirin aktif, katalog, dan semua alokasi lain.

## 6. Kebutuhan fungsional

Prioritas **P0** berarti wajib ada sebelum demonstrasi. **P1** adalah fitur yang tercantum dalam spesifikasi dan dikerjakan setelah seluruh P0 lulus uji.

### 6.1 Core (`maze_core`)

| ID | Kebutuhan | Prioritas | Kriteria penerimaan |
|---|---|---|---|
| MZ-01 | `maze_create` mengalokasikan grid dua dimensi berisi `#`, dan melepas baris yang sudah teralokasi jika alokasi berikutnya gagal | P0 | `ST_NO_MEMORY` dan `util_alloc_balance() == 0` untuk beberapa nilai `k` pada `util_fail_alloc_after(k)` |
| MZ-02 | `maze_destroy` melepas tiap baris, lalu array baris, lalu menjadikan pointer `NULL` | P0 | Pemanggilan ganda aman |
| MZ-03 | `maze_copy` menyalin grid, `S`, dan `E` | P0 | Salinan identik dan independen dari sumber |
| MZ-04 | `maze_print` menampilkan labirin ke layar | P0 | Satu karakter per sel, satu baris layar per baris grid |
| MZ-05 | `maze_load` membaca berkas sesuai format di Bagian 7 | P0 | Berkas sah termuat penuh |
| MZ-06 | `maze_load` menolak berkas rusak dengan `ST_BAD_FORMAT` untuk tujuh kasus di Bagian 7.2 | P0 | Tiap kasus punya satu berkas uji di `data/` |
| MZ-07 | `maze_load` menolak berkas yang tidak ada atau kosong | P0 | `ST_FILE_NOT_FOUND` dan `ST_FILE_EMPTY` |
| MZ-08 | Karakter `*` pada berkas dianggap jalan saat dimuat | P0 | Setelah dimuat, tidak ada `*` pada grid |
| MZ-09 | `maze_save` menulis berkas sesuai format | P0 | Simpan lalu muat menghasilkan grid, `S`, dan `E` yang identik |
| MZ-10 | `maze_count_dead_ends` menghitung sel jalan yang punya tepat satu tetangga jalan, `S` dan `E` tidak dihitung **[Keputusan]** | P1 | Hasil sesuai hitungan manual pada `kecil.txt` |

### 6.2 Katalog (`maze_catalog`)

| ID | Kebutuhan | Prioritas | Kriteria penerimaan |
|---|---|---|---|
| MZ-20 | Memuat katalog dari berkas `nama;file;h;w;kesulitan;best_steps`, satu entri per baris, dan melaporkan jumlah baris rusak lewat `bad_lines` | P0 | Baris rusak tidak menghentikan pemuatan, jumlahnya dilaporkan |
| MZ-21 | Menyimpan katalog ke berkas dengan format yang sama | P0 | Simpan lalu muat menghasilkan isi identik |
| MZ-22 | Menambah entri dengan array dinamis yang diperbesar lewat `realloc` | P0 | Penambahan melewati kapasitas awal tidak merusak data |
| MZ-23 | Mengurutkan menurut ukuran (`h*w`) dengan Bubble Sort | P0 | Terurut naik |
| MZ-24 | Mengurutkan menurut kesulitan dengan Selection Sort | P0 | Terurut naik, 1 sampai 5 |
| MZ-25 | Mengurutkan menurut nama dengan Selection Sort, lalu mengatur `sorted_by_name = 1` | P0 | Terurut alfabetis tanpa membedakan huruf besar dan kecil |
| MZ-26 | Pengurutan menurut ukuran atau kesulitan mengatur `sorted_by_name = 0` dan `maze_catalog_add` juga mengaturnya ke 0 **[Keputusan]** | P0 | Binary Search tidak pernah berjalan pada data yang tidak terurut menurut nama |
| MZ-27 | Pencarian nama dengan Sequential Search | P0 | Mengembalikan indeks atau `-1` |
| MZ-28 | Pencarian nama dengan Binary Search, mengembalikan `-2` jika katalog belum terurut menurut nama | P0 | Hasil benar setelah pengurutan nama, `-2` sebelumnya |
| MZ-29 | Menu katalog menampilkan tabel entri, mendukung urut, cari, tambah, dan buka (memuat labirin entri itu sebagai labirin aktif) | P0 | Seluruh alur dapat dijalankan dari menu |

### 6.3 Generator (`maze_gen`)

| ID | Kebutuhan | Prioritas | Kriteria penerimaan |
|---|---|---|---|
| MZ-30 | `maze_gen_backtracker` membangkitkan labirin dengan *recursive backtracker* | P0 | Labirin sempurna (`loops = 0`) memenuhi syarat Bagian 12 |
| MZ-31 | `maze_gen_prim` membangkitkan labirin dengan *randomized Prim* memakai array frontier dinamis | P0 | Sama seperti MZ-30 |
| MZ-32 | `h` dan `w` harus ganjil dan berada pada 5 sampai 61, selain itu `ST_OUT_OF_RANGE` | P0 | Uji dengan nilai genap, 3, dan 63 |
| MZ-33 | `S` ditempatkan di (1,1) dan `E` di (h-2, w-2) | P0 | Terperiksa pada setiap labirin hasil generator |
| MZ-34 | Seed yang sama menghasilkan grid identik. `srand(seed)` dipanggil di dalam generator, dan tidak di tempat lain | P0 | Dua panggilan dengan seed sama menghasilkan grid identik, untuk kedua algoritma |
| MZ-35 | Parameter `loops` membuka sejumlah dinding interior yang berada di antara dua sel jalan. Jika kandidat kurang dari `loops`, semua kandidat dibuka tanpa error **[Keputusan]** | P0 | Jumlah sel jalan bertambah sebanyak dinding yang dibuka |
| MZ-36 | Menu generate meminta algoritma, `h`, `w`, seed, dan `loops`, lalu menampilkan hasil dan menjadikannya labirin aktif | P0 | Input genap atau di luar rentang ditolak dengan pesan |

### 6.4 Solver (`maze_solve`)

| ID | Kebutuhan | Prioritas | Kriteria penerimaan |
|---|---|---|---|
| MZ-40 | `maze_solve_dfs` mencari jalur secara rekursif dengan array `visited`, dan jalur dibangun saat rekursi kembali | P0 | `found = 1` pada labirin yang dapat diselesaikan |
| MZ-41 | `maze_solve_bfs` memakai antrean berupa array indeks sel dan array `parent`, lalu merekonstruksi jalur mundur dari `E` | P0 | Jalur terpendek, panjangnya cocok dengan hitungan manual |
| MZ-42 | `maze_solve_wall` mengikuti dinding kanan dengan urutan belok kanan, lurus, belok kiri, putar balik, dan berhenti setelah `4*h*w` langkah | P0 | Berhasil pada labirin sempurna, `found = 0` pada contoh `loop_putar.txt` |
| MZ-43 | Semua solver mengisi `MazeSolveResult` (`found`, `length`, `visited`) dan jalur dialokasikan dinamis, yang pemanggilnya wajib melepas | P0 | `util_alloc_balance() == 0` setelah jalur dilepas |
| MZ-44 | Ketiga solver menghasilkan `found = 0` pada labirin tanpa jalur, tanpa jalur yang bocor | P0 | Berlaku untuk `tanpa_jalan.txt` |
| MZ-45 | `maze_draw_path` menandai jalur dengan `*` kecuali pada `S` dan `E` | P0 | `S` dan `E` tetap tampil |
| MZ-46 | Menu solve menawarkan satu solver atau ketiganya sekaligus, menampilkan labirin berjalur beserta `found`, `length`, dan `visited` | P0 | Pembanding ketiga solver tampil dalam satu layar |

### 6.5 Editor (`maze_editor`)

| ID | Kebutuhan | Prioritas | Kriteria penerimaan |
|---|---|---|---|
| MZ-50 | `maze_run_editor` membaca perintah dengan `fgets`, memecahnya dengan `strtok` atau `sscanf`, dan tidak peka huruf besar dan kecil | P0 | `WALL 3 4` sama dengan `wall 3 4` |
| MZ-51 | Perintah `new h w` membuat labirin kosong (tepi dinding, dalam jalan) | P0 | Dimensi di luar 5 sampai 61 ditolak |
| MZ-52 | `wall r c` dan `open r c` mengubah sel, ditolak jika sel berada di tepi, di luar grid, atau berupa `S` atau `E` **[Keputusan]** | P0 | Pesan error jelas, grid tidak berubah |
| MZ-53 | `start r c` dan `end r c` memindahkan `S` dan `E`, sel tujuan menjadi jalan | P0 | Sel tujuan di tepi, di luar grid, atau berupa titik lain ditolak |
| MZ-54 | `show` menampilkan labirin dengan penomoran baris dan kolom | P0 | Penomoran membantu menentukan koordinat `wall` dan `open` |
| MZ-55 | `check` menjalankan validasi dan BFS, lalu melaporkan dapat diselesaikan atau tidak serta panjang jalur | P0 | Laporan sesuai kondisi labirin |
| MZ-56 | `undo` membatalkan perintah ubah terakhir memakai stack dinamis berisi `{jenis, r, c, nilai_sebelumnya}`, dan `new` serta `load` mengosongkan stack | P0 | Rangkaian perintah lalu `undo` mengembalikan keadaan semula |
| MZ-57 | `save nama.txt` dan `load nama.txt` menyimpan dan memuat labirin | P0 | Berkas rusak tidak mengubah labirin yang sedang disunting |
| MZ-58 | `help` mencantumkan perintah dan `quit` keluar dari editor | P0 | Daftar sama dengan Bagian 8.3 |
| MZ-59 | Perintah tidak dikenal, argumen kurang, argumen bukan angka, dan koordinat tidak sah menghasilkan pesan, tanpa crash | P0 | Uji dengan baris kosong, `wall`, `wall a b`, dan `fly 1 1` |

### 6.6 Validator (`maze_validate`)

| ID | Kebutuhan | Prioritas | Kriteria penerimaan |
|---|---|---|---|
| MZ-60 | Memeriksa seluruh sel tepi adalah `#` | P0 | Pelanggaran dilaporkan pada `msg` |
| MZ-61 | Memeriksa tepat satu `S` dan tepat satu `E` | P0 | Pelanggaran dilaporkan pada `msg` |
| MZ-62 | Menjalankan *flood fill* rekursif dari `S`, lalu memeriksa `E` terjangkau | P0 | `E` yang terkurung terdeteksi |
| MZ-63 | Membandingkan jumlah sel jalan yang terjangkau dengan jumlah seluruh sel jalan, selisihnya adalah sel terisolasi | P0 | Contoh `terisolasi.txt` melaporkan tepat satu sel terisolasi |
| MZ-64 | Nilai kembali: `ST_OK` jika tepi, `S`, `E`, dan keterjangkauan `E` semuanya benar, selain itu `ST_BAD_FORMAT`. Sel terisolasi disebutkan pada `msg` tanpa mengubah status **[Keputusan]** | P0 | `msg` memuat kata "terisolasi" dan jumlahnya |

### 6.7 Benchmark (`maze_bench`)

| ID | Kebutuhan | Prioritas | Kriteria penerimaan |
|---|---|---|---|
| MZ-70 | `maze_bench_run` membuat, untuk tiap seed berurutan dari `base_seed`, satu labirin per generator, lalu menjalankan tiga solver pada tiap labirin | P0 | Hasil berisi tepat `n × 2 × 3` baris |
| MZ-71 | `maze_bench_summary` menampilkan, per kombinasi generator dan solver: jumlah percobaan, jumlah berhasil, rata-rata, minimum, dan maksimum panjang jalur (hanya yang berhasil), rata-rata sel dikunjungi, dan persentase gagal | P0 | Cocok dengan hitungan manual pada kasus kecil |
| MZ-72 | Histogram teks panjang jalur BFS | P1 | Lihat Bagian 8.4 |
| MZ-73 | Mengurutkan baris menurut sel dikunjungi dengan Bubble Sort dan menurut panjang jalur dengan Selection Sort | P0 | Terurut naik |
| MZ-74 | Mencari seed pada baris hasil dengan Sequential Search | P0 | Mengembalikan indeks atau `-1` |
| MZ-75 | `maze_bench_write_report` menulis laporan ke berkas teks | P0 | Isi laporan sama dengan ringkasan di layar |
| MZ-76 | Jumlah labirin `n` berkisar 1 sampai 1000 pada menu **[Keputusan]** | P0 | Nilai di luar rentang ditolak |
| MZ-77 | Kegagalan alokasi saat benchmark melepas semua baris yang sudah terisi dan mengembalikan `ST_NO_MEMORY` | P0 | `util_alloc_balance() == 0` |

## 7. Format berkas

### 7.1 Berkas labirin

```
11 21
#####################
#S....#.............#
#.###.#.#####.#####.#
...
#####################
```

Baris pertama berisi `h w`, keduanya antara 5 dan 61. Selanjutnya ada `h` baris yang masing-masing sepanjang `w` karakter.

| Karakter | Arti |
|---|---|
| `#` | Dinding |
| `.` | Jalan |
| `S` | Titik awal |
| `E` | Titik tujuan |
| `*` | Jalur solusi, hanya pada keluaran. Saat dimuat dianggap jalan |

Aturan sah: seluruh sel tepi adalah `#`, ada tepat satu `S`, dan ada tepat satu `E`. Pemuatan hanya memeriksa kesesuaian format. Apakah labirin dapat diselesaikan adalah urusan validator dan solver, sehingga `tanpa_jalan.txt` tetap termuat dengan sukses.

### 7.2 Tujuh kasus berkas rusak

Setiap kasus menghasilkan `ST_BAD_FORMAT` dan mempunyai satu berkas uji.

| Berkas uji | Kasus |
|---|---|
| `rusak_tanpa_s.txt` | Tidak ada `S` |
| `rusak_dua_e.txt` | Ada dua `E` |
| `rusak_beda_panjang.txt` | Satu baris lebih pendek atau lebih panjang dari `w` |
| `rusak_karakter.txt` | Memuat karakter di luar `# . S E *` |
| `rusak_tepi.txt` | Tepi tidak seluruhnya `#` |
| `rusak_dimensi.txt` | `h` atau `w` di luar 5 sampai 61 |
| `rusak_jumlah_baris.txt` | Jumlah baris tidak cocok dengan `h` pada header |

### 7.3 Berkas katalog

Satu entri per baris, dipisahkan titik koma:

```
nama;file;h;w;kesulitan;best_steps
```

Contoh isi `library.txt`:

```
Kecil;04-maze-lab/data/kecil.txt;6;7;1;7
Putar;04-maze-lab/data/loop_putar.txt;8;9;2;6
Tanpa Jalan;04-maze-lab/data/tanpa_jalan.txt;6;7;1;-1
```

`kesulitan` bernilai 1 sampai 5. `best_steps` bernilai `-1` jika belum diketahui. Baris dengan jumlah kolom salah, angka tidak valid, `h` atau `w` di luar rentang, atau kesulitan di luar 1 sampai 5 dihitung sebagai baris rusak. Berkas `katalog_rusak.txt` memuat contoh baris semacam itu.

## 8. Contoh keluaran

### 8.1 Labirin kecil dan jalur BFS

`kecil.txt` (6 × 7), jalur tunggal sepanjang 7 langkah:

```
#######        #######
#S..#.#        #S**#.#
#.#.#.#   ->   #.#*#.#
#.#...#        #.#***#
#.###E#        #.###E#
#######        #######
```

### 8.2 Contoh yang dirancang untuk uji tertentu

Seluruh angka berikut dihitung dengan simulasi terpisah sebelum dokumen ini ditulis, memakai arah awal *wall follower* ke timur dan urutan belok kanan, lurus, kiri, putar balik. Program C tetap harus membuktikannya lewat `test_maze_lab()`.

| Berkas | Isi | Hasil yang diharapkan |
|---|---|---|
| `kecil.txt` | Pada 8.1 | BFS: `found = 1`, `length = 7` |
| `loop_putar.txt` | Ruang terbuka 8 × 9 dengan satu pilar di tengah, `E` menempel pada pilar | BFS: `found = 1`, `length = 6`. *Wall follower*: `found = 0` setelah mencapai batas `4*h*w = 288` langkah |
| `tanpa_jalan.txt` | `E` terkurung dinding | Ketiga solver: `found = 0` |
| `terisolasi.txt` | Satu sel jalan terkurung di luar jalur `S` ke `E` | Validator: `E` terjangkau, tepat satu sel terisolasi. BFS: `found = 1`, `length = 4` |

Isi `loop_putar.txt`:

```
#########
#S......#
#.......#
#...#...#
#...E...#
#.......#
#.......#
#########
```

Isi `tanpa_jalan.txt`:

```
#######
#S..#.#
#...#.#
#####E#
#.....#
#######
```

Isi `terisolasi.txt`:

```
#######
#S...E#
#####.#
#.#...#
#######
```

Panjang jalur DFS bergantung pada urutan arah yang dipilih, sehingga uji hanya memeriksa bahwa panjang DFS tidak lebih kecil dari panjang BFS.

### 8.3 Sesi editor

```
> new 11 21
> wall 3 4
> start 1 1
> end 9 19
> check
Labirin dapat diselesaikan. Panjang jalur terpendek: <angka> langkah.
> undo
> wall 0 5
Ditolak: sel (0,5) berada di tepi labirin.
> save labirin1.txt
> quit
```

Koordinat dihitung dari nol, dengan baris lebih dulu lalu kolom. Pada labirin 11 × 21, sel `(9,19)` adalah sel dalam di pojok kanan bawah.

### 8.4 Ringkasan benchmark (tata letak)

```
=== RINGKASAN BENCHMARK (n=<n>, ukuran <h>x<w>, loops=<k>) ===
Generator     Solver   Coba  Berhasil  Rata2  Min  Maks  Rata2-dikunjungi  Gagal(%)
backtracker   dfs      ...
backtracker   bfs      ...
backtracker   wall     ...
prim          dfs      ...
prim          bfs      ...
prim          wall     ...

Histogram panjang jalur BFS
[ 20 -  39] ##########  10
[ 40 -  59] ################  16
...
```

Rentang histogram dibagi dalam sepuluh kelas sama lebar antara panjang minimum dan maksimum jalur BFS. **[Keputusan]**

## 9. Desain teknis

### 9.1 Struktur folder

```
04-maze-lab/
├── maze_lab.h/.c        run_maze_lab, test_maze_lab, menu
├── maze_core.h/.c       tipe Maze, create/destroy/copy/print, load/save, validator
├── maze_catalog.h/.c    katalog labirin (sorting/searching)
├── maze_gen.h/.c        generator
├── maze_solve.h/.c      solver
├── maze_editor.h/.c     editor berbasis perintah teks
├── maze_bench.h/.c      benchmark dan laporan
├── data/                library.txt, kecil.txt, tanpa_jalan.txt, loop_putar.txt,
│                        terisolasi.txt, katalog_rusak.txt, rusak_*.txt
└── README.md
```

### 9.2 Struct

```c
typedef struct { int r, c; } MazeCell;
typedef struct { int h, w; char **grid; MazeCell start, end; } Maze;
typedef struct { int found; int length; int visited; } MazeSolveResult;

typedef struct {
    char name[32]; char file[UTIL_MAX_PATH];
    int h, w; int difficulty;        /* 1..5 */
    int best_steps;                  /* -1 jika belum diketahui */
} MazeInfo;
typedef struct { MazeInfo *items; int count; int capacity; int sorted_by_name; } MazeCatalog;

typedef struct {
    unsigned seed; char gen[16]; char solver[16];
    int found, length, visited;
} MazeBenchRow;
```

`length` pada `MazeSolveResult` adalah jumlah langkah, sehingga jalur yang disimpan berisi `length + 1` sel.

### 9.3 Fungsi publik

| Modul | Fungsi | Materi RPS yang tercakup |
|---|---|---|
| `maze_core` | `maze_create`, `maze_destroy`, `maze_copy`, `maze_print`, `maze_load`, `maze_save`, `maze_validate`, `maze_count_dead_ends` | Array dua dimensi, pointer ke pointer, file teks, error handling |
| `maze_catalog` | `maze_catalog_load`, `_save`, `_add`, `_sort_by_size`, `_sort_by_difficulty`, `_sort_by_name`, `_find_name_seq`, `_find_name_binary`, `_free` | Array of struct, Bubble Sort, Selection Sort, Sequential Search, Binary Search |
| `maze_gen` | `maze_gen_backtracker`, `maze_gen_prim` | Rekursi, bilangan acak, array dinamis |
| `maze_solve` | `maze_solve_dfs`, `maze_solve_bfs`, `maze_solve_wall`, `maze_draw_path` | Rekursi, antrean, array `parent`, struct sebagai nilai kembali |
| `maze_editor` | `maze_run_editor` | Pemrosesan string, `switch`, stack dinamis |
| `maze_bench` | `maze_bench_run`, `_summary`, `_sort_by_visited`, `_sort_by_length`, `_find_seed`, `_write_report` | Array of struct, sorting, searching, file teks |

Prototipe lengkap mengikuti Bagian 2.4 dokumen sumber.

### 9.4 Cakupan sorting dan searching

| Algoritma | Dipakai pada |
|---|---|
| Bubble Sort | Katalog menurut ukuran, baris benchmark menurut sel dikunjungi |
| Selection Sort | Katalog menurut kesulitan dan nama, baris benchmark menurut panjang jalur |
| Sequential Search | Katalog menurut nama, benchmark menurut seed |
| Binary Search | Katalog menurut nama, hanya jika `sorted_by_name == 1` |

### 9.5 Algoritma

- **Backtracker.** Sel logis berada pada koordinat ganjil. Mulai dari (1,1), urutan empat arah diacak dengan Fisher-Yates, lalu lompat dua sel, buka dinding di antaranya, dan rekursi. Kedalaman rekursi maksimal sekitar ((h-1)/2) × ((w-1)/2), atau paling banyak 900 pada labirin 61 × 61.
- **Prim acak.** Dinding frontier disimpan di array dinamis. Satu dinding dipilih acak. Jika sel di seberangnya belum dikunjungi, dinding dibuka dan frontier baru ditambahkan.
- **`loops`.** Sesudah generasi selesai, `loops` dinding interior acak yang diapit dua sel jalan dibuka, sehingga tercipta jalur alternatif.
- **DFS rekursif.** Array `visited` bertipe `char *` berukuran h × w, urutan arah tetap (atas, kanan, bawah, kiri), dan jalur dibangun saat rekursi kembali. `visited` menghitung sel yang dimasuki.
- **BFS.** Antrean berupa array `int` berisi indeks sel (`r * w + c`) berukuran h × w, ditambah array `parent`. Jalur direkonstruksi mundur dari `E`. `visited` menghitung sel yang dikeluarkan dari antrean. **[Keputusan]**
- **Wall follower.** Arah awal ke timur. Pada tiap langkah, program mencoba belok kanan, lurus, belok kiri, lalu putar balik, dan memakai arah pertama yang terbuka. Pencarian berhenti jika sampai di `E` atau melewati `4*h*w` langkah (`found = 0`). Metode ini menjamin hasil pada labirin sempurna dan dapat gagal jika ada jalur melingkar. `visited` menghitung sel berbeda yang pernah dilewati. **[Keputusan]**
- **Validator.** Memeriksa tepi, menghitung `S` dan `E`, menjalankan *flood fill* rekursif dari `S`, lalu membandingkan jumlah sel jalan terjangkau dengan jumlah seluruh sel jalan. Selisihnya adalah sel terisolasi.
- **Undo.** Entri stack berisi `{jenis, r, c, nilai_sebelumnya}`. Untuk `wall` dan `open`, `(r, c)` adalah sel yang diubah dan `nilai_sebelumnya` adalah isinya semula. Untuk `start` dan `end`, `(r, c)` adalah posisi lama dan `nilai_sebelumnya` adalah isi sel tujuan sebelum ditimpa. Posisi baru diambil dari `m->start` atau `m->end` saat `undo`, karena pembatalan berjalan berurutan dari yang terbaru. **[Keputusan]**

### 9.6 Manajemen memori

| Alokasi | Pemilik | Fungsi pelepas |
|---|---|---|
| Grid dan salinan labirin | Pemanggil `maze_create` atau `maze_copy` | `maze_destroy` |
| Array path, `visited`, `parent`, antrean | Solver (path diserahkan ke pemanggil) | `util_free` |
| Frontier Prim | `maze_gen_prim` | Dilepas di akhir fungsi, termasuk jalur error |
| Katalog | Pemanggil `maze_catalog_load` | `maze_catalog_free` |
| Stack undo | `maze_run_editor` | Dilepas saat editor selesai |
| Baris benchmark | Pemanggil `maze_bench_run` | `util_free` |

Aturan: semua alokasi lewat `util_malloc`, `util_calloc`, dan `util_realloc`. Hasil `NULL` dikembalikan sebagai `ST_NO_MEMORY`, semua alokasi yang sudah berhasil dilepas di jalur error, hasil `realloc` ditampung di variabel sementara lebih dulu, pointer diatur ke `NULL` setelah `free`, dan parameter yang hanya dibaca diberi `const`.

### 9.7 Ketergantungan pada pustaka bersama

Maze Lab memakai `common/common.h` (`Status`, `UTIL_MAX_LINE`, `UTIL_MAX_PATH`) dan fungsi `util_read_line`, `util_read_int`, `util_trim`, `util_to_lower`, `util_stricmp`, `util_read_file`, `util_write_file`, `util_malloc` sampai `util_free`, `util_alloc_balance`, `util_fail_alloc_after`, serta fungsi *test harness* (`util_test_begin`, `util_check`, `util_check_double`, `util_test_end`). `srand` hanya dipanggil di dalam generator.

## 10. Kebutuhan nonfungsional

| Aspek | Kebutuhan |
|---|---|
| Bahasa dan kompilator | C99, kompilasi dengan `-std=c99 -Wall -Wextra -Wpedantic -g`, nol *warning*, tautan `-lm` |
| Portabilitas | Dapat dikompilasi di Windows (MinGW-w64 gcc) dan Linux, tanpa fungsi khusus POSIX atau Windows |
| Gaya kode | `snake_case`, fungsi di bawah 50 baris, satu file `.c` di bawah 400 baris, tanpa `goto`, tanpa variabel global |
| Dokumentasi kode | Komentar bahasa Indonesia di atas tiap fungsi: tujuan, parameter, nilai kembali, dan siapa yang melepas memori |
| Ketahanan rekursi | Kedalaman DFS pada labirin terbuka penuh dapat mencapai 3.481 sel (59 × 59). Frame fungsi harus kecil dan array besar dialokasikan di memori dinamis |
| Reproduksibilitas | Seed yang sama menghasilkan grid identik pada satu platform. Hasil `rand()` dapat berbeda antarplatform, sehingga berkas contoh disimpan sebagai berkas, bukan diharapkan muncul kembali dari seed |
| Memori | Tidak ada kebocoran, dibuktikan oleh `util_alloc_balance()`, lalu oleh `valgrind` atau `-fsanitize=address` di Linux atau WSL |
| Berkas | Hanya berkas teks. Path relatif terhadap folder root, misalnya `04-maze-lab/data/kecil.txt` |
| Performa | Benchmark n = 1000 pada 61 × 61 selesai dalam waktu yang wajar di laptop biasa |

## 11. Penanganan error

| Situasi | Status | Perilaku |
|---|---|---|
| Berkas tidak ditemukan | `ST_FILE_NOT_FOUND` | Pesan, labirin aktif tidak berubah |
| Berkas kosong | `ST_FILE_EMPTY` | Pesan, labirin aktif tidak berubah |
| Format labirin rusak | `ST_BAD_FORMAT` | Pesan menyebut alasan, labirin aktif tidak berubah |
| Gagal menulis berkas | `ST_FILE_WRITE` | Pesan |
| Ukuran generator genap atau di luar rentang | `ST_OUT_OF_RANGE` | Pesan, minta ukuran lagi |
| Koordinat editor di luar grid atau di tepi | `ST_OUT_OF_RANGE` | Pesan, grid tidak berubah |
| Perintah editor salah, argumen kurang | `ST_BAD_INPUT` | Pesan dan petunjuk `help` |
| Alokasi gagal | `ST_NO_MEMORY` | Semua alokasi sebelumnya dilepas, pesan |
| Binary Search pada katalog belum terurut menurut nama | Nilai kembali `-2` | Menu menawarkan pengurutan nama lebih dulu |

Pesan dihasilkan oleh `util_status_message`, sehingga bunyinya seragam di seluruh menu.

## 12. Data uji dan rencana pengujian

### 12.1 Berkas contoh (`data/`)

| Berkas | Tujuan |
|---|---|
| `library.txt` | Katalog contoh, minimal sepuluh entri dengan ukuran, kesulitan, dan nama yang bervariasi |
| `kecil.txt` | Labirin buatan tangan dengan jawaban diketahui (Bagian 8.1) |
| `loop_putar.txt` | Kasus *wall follower* gagal sementara BFS berhasil |
| `tanpa_jalan.txt` | Labirin tanpa jalur |
| `terisolasi.txt` | Validator mendeteksi sel terisolasi |
| `katalog_rusak.txt` | Katalog yang memuat baris rusak |
| `rusak_*.txt` | Tujuh berkas pada Bagian 7.2 |

### 12.2 Kasus uji minimum

Seluruh baris berikut harus menjadi pemeriksaan `util_check` di `test_maze_lab()`. Jika program menghasilkan nilai berbeda, periksa kodenya lebih dulu sebelum mengubah nilai harapan.

| ID uji | Uji | Hasil yang diharapkan | Kebutuhan |
|---|---|---|---|
| U01 | Simpan lalu muat | Grid, `S`, dan `E` identik | MZ-09 |
| U02 | Muat tujuh berkas rusak | `ST_BAD_FORMAT` untuk masing-masing | MZ-06 |
| U03 | Generator dengan seed sama | Grid identik, untuk kedua algoritma | MZ-34 |
| U04 | Labirin sempurna (`loops = 0`) | Jumlah sel jalan = 2L − 1 dengan L = ((h−1)/2) × ((w−1)/2), semua sel terhubung, `S` dan `E` terjangkau | MZ-30, MZ-31 |
| U05 | `kecil.txt` | BFS menemukan jalur sepanjang 7 langkah | MZ-41 |
| U06 | BFS lawan DFS pada labirin yang dapat diselesaikan | Panjang BFS ≤ panjang DFS, keduanya `found = 1` | MZ-40, MZ-41 |
| U07 | `tanpa_jalan.txt` | Ketiga solver `found = 0` | MZ-44 |
| U08 | Wall follower pada labirin sempurna | `found = 1` | MZ-42 |
| U09 | Wall follower pada `loop_putar.txt` | `found = 0`, sementara BFS `found = 1` dengan panjang 6 | MZ-42 |
| U10 | Validator pada `terisolasi.txt` dan `tanpa_jalan.txt` | Satu sel terisolasi terdeteksi, dan `E` tak terjangkau terdeteksi | MZ-62, MZ-63 |
| U11 | Editor: rangkaian perintah lalu `undo` | Grid sesuai dan keadaan kembali ke semula setelah `undo` | MZ-56 |
| U12 | Editor: perintah salah, argumen kurang, koordinat di luar atau di tepi | Semuanya ditolak, grid tidak berubah | MZ-52, MZ-59 |
| U13 | Katalog: sort ukuran, kesulitan, nama | Urutan benar untuk masing-masing | MZ-23 sampai MZ-25 |
| U14 | Katalog: Binary Search | Benar setelah sort nama, `-2` jika belum terurut menurut nama | MZ-28 |
| U15 | Katalog rusak | Baris rusak dilaporkan lewat `bad_lines` | MZ-20 |
| U16 | Benchmark n = 1 dan n besar | Jumlah baris = n × 2 × 3 | MZ-70 |
| U17 | Ringkasan benchmark kasus kecil | Cocok dengan hitungan manual | MZ-71 |
| U18 | Pencarian seed pada benchmark | Seed yang ada ditemukan, yang tidak ada menghasilkan `-1` | MZ-74 |
| U19 | `maze_create` dengan `util_fail_alloc_after(k)` untuk beberapa k | `ST_NO_MEMORY` dan `util_alloc_balance() == 0` | MZ-01 |
| U20 | Akhir test | `util_alloc_balance() == 0` | S4 |

Uji tambahan yang dianjurkan: ukuran generator genap, 3, dan 63; `loops` melebihi jumlah kandidat; `maze_draw_path` tidak menimpa `S` dan `E`; solver pada labirin 61 × 61; serta pemanggilan `maze_destroy` dua kali.

### 12.3 Uji menu interaktif

Alur menu diuji dengan input terpipa, misalnya `printf "2\n1\n21\n41\n7\n0\n3\n2\n0\n0\n" | ./suite`. Tiga skenario minimum:

1. Generate labirin, selesaikan dengan BFS, simpan, keluar.
2. Buka labirin dari katalog, urutkan menurut nama, cari dengan Binary Search.
3. Editor: `new`, beberapa `wall`, `check`, `undo`, `quit`, ditambah beberapa perintah salah.

### 12.4 Kriteria selesai

- [ ] `make` menghasilkan satu *executable* tanpa *warning*, dan `build.bat` setara.
- [ ] Seluruh kebutuhan P0 lulus kriteria penerimaannya.
- [ ] Seluruh baris U01 sampai U20 ada di kode dan lulus.
- [ ] `./suite --test` menghasilkan kode keluar 0.
- [ ] Input salah, berkas rusak, dan gagal alokasi tidak pernah membuat program crash atau bocor.
- [ ] Folder `data/` lengkap dan `README.md` projek terisi (tujuan, cara menjalankan, format berkas, daftar fungsi publik, daftar uji, keputusan desain).
- [ ] Tidak ada *stub* atau `TODO` tersisa.

## 13. Keterlacakan terhadap RPS

| Sub-CPMK | Materi | Terpenuhi lewat |
|---|---|---|
| 1 | I/O, tipe data, operator, boolean | Menu, `util_read_*`, ringkasan benchmark |
| 2 | `if`, `switch`, perulangan, *nested loop*, `break`, `continue` | Menu dan editor (`switch`), pemindaian grid dengan perulangan bersarang |
| 3 | Fungsi, parameter, return, rekursi, modular | Backtracker, DFS, *flood fill*, enam modul |
| 4 | Array, string, pointer, struct, array of struct | Grid `char **`, `MazeInfo[]`, `MazeBenchRow[]`, parser editor |
| 5 | Bubble Sort, Selection Sort, Sequential Search, Binary Search | Katalog dan benchmark (Bagian 9.4) |
| 6 | File teks, penanganan error, *test case* | `maze_load`, `maze_save`, katalog, laporan, `test_maze_lab()` |
| Bahan kajian 14 | Manajemen memori | `util_malloc` sampai `util_free`, rollback `maze_create`, pelacak alokasi |

## 14. Pembagian tugas

### 14.1 Pembagian lima anggota

| No. | Peran | Tanggung jawab | Berkas | Bergantung pada |
|---|---|---|---|---|
| 1 | Core, katalog, integrasi | Tipe `Maze`, muat dan simpan, katalog, *test harness*, `maze_lab.c`, integrasi akhir | `maze_core.c`, `maze_catalog.c`, `maze_lab.c`, `common/` | Tidak ada |
| 2 | Generator | Backtracker, Prim, `loops`, uji seed | `maze_gen.c` | `maze_create` |
| 3 | Solver | DFS, BFS, `maze_draw_path` | `maze_solve.c` | `Maze`, `maze_create` |
| 4 | Editor, validator, wall follower | Editor dengan `undo`, `maze_validate`, `maze_count_dead_ends`, `maze_solve_wall` | `maze_editor.c`, bagian `maze_core.c`, bagian `maze_solve.c` | `Maze`, `maze_solve_bfs` untuk `check` |
| 5 | Benchmark dan laporan | `maze_bench_*`, histogram, laporan | `maze_bench.c` | Generator dan solver |

### 14.2 Penyetelan beban

Modul generator dan solver paling berat, dan sumber menyarankan agar pemegang tugas terberat tidak mengurus integrasi akhir. Karena itu pembagian di atas memindahkan *wall follower* dari Anggota 3 ke Anggota 4, sehingga Anggota 3 hanya memegang DFS dan BFS. Jika Anggota 2 masih kewalahan, Prim juga dapat dipindahkan ke Anggota 5 yang modulnya baru dapat dimulai setelah generator dan solver tersedia. **[Keputusan]**

Dua berkas dipegang bersama:

- `maze_core.c` dimiliki Anggota 1. Fungsi `maze_validate` dan `maze_count_dead_ends` ditulis Anggota 4, lalu digabungkan lewat peninjauan.
- `maze_solve.c` dimiliki Anggota 3. Fungsi `maze_solve_wall` ditulis Anggota 4 dengan cara yang sama.

### 14.3 Tahap pengerjaan

Tahap berikut disusun berurutan dan durasinya disesuaikan dengan tenggat dari dosen.

| Tahap | Isi | Keluaran |
|---|---|---|
| T1 Fondasi | `common/`, `Maze`, `maze_create`, `maze_destroy`, `maze_load`, `maze_print`, kerangka menu, build bersih | Labirin dapat dimuat dan ditampilkan |
| T2 Modul paralel | Generator, solver, editor, dan benchmark dikerjakan paralel dengan `test_` lokal | Tiap modul lulus uji sendiri |
| T3 Integrasi | Menu memanggil semua modul, data contoh lengkap, `test_maze_lab()` memuat U01 sampai U20 | `./suite --test` hijau |
| T4 Verifikasi | Pemeriksaan kebocoran (`valgrind` atau ASan), uji menu terpipa, README projek | Semua butir Bagian 12.4 tercentang |
| T5 Latihan demo | Tiap anggota menjelaskan bagian yang dipegangnya dan menjawab pertanyaan silang | Naskah demo dan daftar pertanyaan antisipasi |

## 15. Skenario demonstrasi yang disarankan

1. Bangkitkan labirin 21 × 41 dengan backtracker dan seed tertentu, lalu tampilkan.
2. Selesaikan dengan DFS, BFS, dan *wall follower*, kemudian bandingkan `length` dan `visited` pada satu layar.
3. Muat `loop_putar.txt` dan tunjukkan *wall follower* gagal sementara BFS berhasil. Ini jawaban atas pertanyaan penguji tentang batas tiap algoritma.
4. Buka editor, ubah labirin, jalankan `check`, lalu `undo`.
5. Jalankan benchmark kecil dan tunjukkan ringkasan beserta histogram.
6. Tunjukkan `./suite --test` dengan semua uji lulus dan keseimbangan alokasi nol.

## 16. Keputusan dan pertanyaan terbuka

### 16.1 Keputusan yang diambil di dokumen ini

| Keputusan | Pilihan | Alasan |
|---|---|---|
| Arah awal *wall follower* | Timur | Sumber hanya menyebut "arah awal", dan arah menentukan hasil uji `loop_putar.txt` |
| Hitungan `visited` | DFS: sel dimasuki. BFS: sel keluar antrean. Wall: sel berbeda yang dilewati | Sumber tidak menetapkan, dan ketiganya sederhana dihitung |
| Nilai kembali `maze_validate` | `ST_BAD_FORMAT` untuk pelanggaran tepi, `S`, `E`, atau `E` tak terjangkau. Sel terisolasi hanya dilaporkan | Labirin dengan sel terisolasi tetap dapat diselesaikan |
| `maze_count_dead_ends` | `S` dan `E` tidak dihitung | Keduanya sengaja berujung |
| `wall` dan `open` pada `S` atau `E` | Ditolak | Menjaga syarat tepat satu `S` dan satu `E` |
| `loops` melebihi kandidat | Semua kandidat dibuka, tanpa error | Menghindari kegagalan pada labirin kecil |
| Rentang `n` benchmark | 1 sampai 1000 | Menjaga waktu dan memori tetap wajar |
| Kelas histogram | Sepuluh kelas sama lebar | Sederhana dan cukup informatif |
| Pindah *wall follower* ke Anggota 4 | Ya | Menyeimbangkan beban (Bagian 14.2) |
| Undo untuk `start` dan `end` | Posisi lama disimpan pada `(r, c)` | Memakai struktur entri yang sudah ada |

Tiap keputusan dicatat juga di `README.md` projek.

### 16.2 Perlu dikonfirmasi ke dosen atau asisten

1. Apakah `malloc`, `realloc`, dan `free` boleh dipakai. Bahan kajian nomor 14 menyebut manajemen memori, tetapi rencana mingguan tidak memuatnya. Jika tidak boleh, array dinamis diganti array berukuran tetap (grid maksimum 61 × 61) dan pelacak alokasi dihapus.
2. Apakah kode boleh dipecah ke beberapa file `.c` dan `.h`. Jika tidak boleh, file digabung per projek dengan pemisah bagian yang rapi.

## 17. Risiko dan mitigasi

| Risiko | Dampak | Mitigasi |
|---|---|---|
| Generator dan solver terlalu berat bagi satu orang | Tenggat mundur, integrasi terlambat | Pindahkan *wall follower* (Bagian 14.2), mulai dari backtracker lebih dulu, dan Prim menyusul |
| Pemegang integrasi akhir juga memegang tugas terberat | Integrasi macet | Integrasi dipegang Anggota 1 |
| Indeks di luar grid saat memeriksa tetangga | Crash atau data rusak | Sel tepi selalu `#`, dan setiap akses tetangga lewat fungsi pemeriksa batas |
| Kedalaman rekursi pada labirin besar | *Stack overflow* | Ukuran dibatasi 61 × 61, frame kecil, array besar di memori dinamis |
| `srand` global memengaruhi komponen lain | Hasil tidak dapat direproduksi | `srand(seed)` hanya di dalam generator |
| Lupa melepas baris grid satu per satu | Kebocoran memori | `maze_destroy` diuji lewat `util_alloc_balance()` dan uji gagal alokasi |
| Jalur `*` menimpa `S` dan `E` | Titik awal dan akhir hilang saat tampil | `maze_draw_path` mengecualikan keduanya, ada uji tambahan |
| *Wall follower* berputar tanpa henti | Program tidak berhenti | Batas `4*h*w` langkah |
| Contoh *loop* yang membuat *wall follower* gagal tidak terbukti | Uji U09 tidak dapat ditulis | `loop_putar.txt` sudah diverifikasi dengan simulasi terpisah, hasilnya tetap diperiksa ulang oleh kode C |
| Anggota tidak memahami bagian lain saat tanya jawab | Nilai individu turun | Latihan demo silang pada tahap T5 |

## Lampiran A. Ringkasan struktur menu utama

```
=== UAS SUITE: DASAR PEMROGRAMAN (C) ===
1. Text Toolkit
2. Kalkulator Ekspresi + Matrix
3. Data Analyzer
4. Maze Lab
5. Jalankan semua self-test
0. Keluar
```

Pilihan 4 memanggil `run_maze_lab()`. Pilihan 5 menjalankan `test_maze_lab()` bersama ketiga uji projek lain, dan argumen `--test` menjalankan semuanya tanpa interaksi.
