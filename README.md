# sdv-qt (Qt 6)

Software diagnostik ZCU/VCU via raw Ethernet (EtherType 0x88B5). Migrasi dari versi WPF.
Data Logging dan Data List mengambil `Steer Remote Raw` dan `Throttle Remote Raw`
langsung dari payload frame logger VCU, serta menampilkan nilai map sesuai konfigurasi
interpolasi VCU.

## Halaman
- **Data List**: halaman yang dibuka langsung saat aplikasi dimulai.
- **Setup Configuration**: pilih adapter, atur MAC tiap device, Test Connection. Pengaturan disimpan (QSettings) dan dapat dibuka dari menu Setup.
- **Data List**: baris = parameter, kolom = corner (FL, FR, RL, RR), refresh 10x per detik.
- **Graph**: sementara dinonaktifkan.

## Build (Linux / embedded Linux)
    sudo apt install cmake g++ qt6-base-dev libqt6charts6-dev libpcap-dev
    cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
    cmake --build build -j
    sudo setcap cap_net_raw,cap_net_admin=eip build/sdv-qt # agar tidak perlu root
    ./build/sdv-qt

## Build (Windows)
Pasang Qt 6 (Widgets + Charts), Npcap, dan Npcap SDK. Set `PCAP_ROOT` ke folder SDK, lalu build dengan CMake.

## Menambah parameter
Tambah satu baris di `src/Params.h`; Data List dan Graph ikut otomatis.

## Format paket (harus sama dengan `zone_data` di firmware, 56 byte, little-endian)
Payload berisi 14 nilai `float` IEEE-754 dalam urutan:
`v_in`, `current_in`, `current_motor`, `erpm`, `temp_fet`, `temp_motor`,
`duty_cycle` (persen), `wh_used`, `wh_charged`, `steer` (derajat),
`stretch` (persen), `speed` (persen), `current` (persen, negatif = arah balik),
dan `temp` (tingkat panas persen, 0 = dingin, 100 = panas).
Nilai `steer` dihitung di firmware berdasarkan `config.h`: titik tengah selalu
0 derajat, sisi kiri negatif, dan sisi kanan positif.

Payload logger VCU berukuran 16 byte: `raw_steer` u32, `raw_speed` u32,
`map_steer` float, dan `map_speed` float. Nilai map dibaca langsung dari VCU;
Qt tidak melakukan mapping ulang.
