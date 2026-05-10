# Tucil 3 STIMA - GUI Ice Sliding Puzzle Solver

Program ini adalah aplikasi **GUI** untuk menyelesaikan puzzle grid berbasis sliding movement menggunakan algoritma:
`UCS`, `BFS`, `GBFS`, `A*`, dan `IDA*`.

## Fitur GUI

- Pemilihan algoritma (`UCS/BFS/GBFS/A*/IDA*`) langsung dari panel konfigurasi.
- Pemilihan heuristic (`H1/H2/H3`) untuk `GBFS`, `A*`, dan `IDA*`.
- Visualisasi board dan posisi agent secara real-time.
- Visualisasi state proses pencarian (iterasi) dan playback solusi.
- Menampilkan metrik hasil: cost, waktu eksekusi, jumlah iterasi, dan move path.
- Menyimpan hasil run ke file output.

## Requirement

- CMake `>= 3.21`
- Compiler C++ dengan dukungan `C++17` (disarankan MSVC)
- `raylib` (akan otomatis diunduh via `FetchContent` jika belum terpasang)

Catatan:
- Build pertama membutuhkan koneksi internet agar dependency raylib dapat diunduh.

## Build

Jalankan dari root project:

```bash
cmake -S . -B .cmake
cmake --build .cmake --config Release
```

Target executable GUI:
- `bin/solver_gui.exe`

## Menjalankan Aplikasi GUI

```bash
.\bin\solver_gui.exe
```

## Cara Menggunakan GUI

1. Pilih algoritma pada bagian **Algorithm Config**.
2. Jika algoritma yang dipilih `GBFS/A*/IDA*`, pilih heuristic (`H1/H2/H3`).
3. Pilih file input map `.txt` (opsional file cost terpisah jika format dipisah).
4. Klik tombol **SOLVE**.
5. Lihat hasil pada panel:
   - **Puzzle Map** untuk visualisasi board.
   - **Iteration Log** untuk jejak iterasi pencarian.
   - **Solution Playback & Metrics** untuk navigasi langkah solusi dan statistik run.
   - **Visualization Options**:
     - **Show Process (iterations)**: menampilkan urutan state yang dikunjungi selama proses pencarian (iterasi algoritma).
     - **Show Solved Playback**: menampilkan urutan langkah dari solusi akhir yang ditemukan (dari start sampai goal).
6. Klik **Save Solution** untuk menyimpan hasil ke file.

## Format Input

Format standar (map + cost dalam satu file):

```text
N M
<N baris grid map>
<N baris nilai cost, masing-masing M integer non-negatif>
```

Contoh:

```text
7 7
XXXXXXX
X0****X
X**X**X
X****OX
X1***LX
XZ**X*X
XXXXXXX
999 999 999 999 999 999 999
999 3   5   2   8   1   999
999 7   4   999 6   9   999
999 2   8   3   5   4   999
999 6   1   7   2   999 999
999 9   3   4   999 8   999
999 999 999 999 999 999 999
```

Arti tile:
- `X` = dinding
- `*` = lantai
- `L` = lava
- `Z` = titik start (harus tepat satu)
- `O` = titik goal (harus tepat satu)
- `0..9` = waypoint yang wajib dikunjungi berurutan dari `0`

## Output File

Hasil penyimpanan dari GUI:
- `test/output/hasil-pencarian-<nama_input>.txt`

File ini berisi ringkasan run, detail iterasi, solusi, cost, waktu eksekusi, dan konfigurasi yang dipilih.

## Troubleshooting

- `solver_gui` tidak terbentuk saat build:
  - Pastikan internet aktif saat build pertama.
  - Jalankan ulang proses build setelah dependency selesai diunduh.
- Gagal memuat input:
  - Periksa format file (`N M`, jumlah baris map, jumlah baris cost).
  - Pastikan karakter map valid dan waypoint berurutan dari `0`.
- Output tidak tersimpan:
  - Pastikan folder `test/output` tersedia atau direktori kerja memiliki izin tulis.

## Author

| Nama | NIM |
|---|---|
| Dika Pramudya Nugraha | 13524132 |
| Daniel Putra Rywandi S | 13524143 |
