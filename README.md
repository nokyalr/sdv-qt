# sdv-qt (Qt 6)

Software diagnostik ZCU/VCU via raw Ethernet (EtherType 0x88B5). Migrasi dari versi WPF.

## Halaman
- **Setup Configuration**: pilih adapter, atur MAC tiap device, Test Connection. Pengaturan disimpan (QSettings).
- **Data List**: baris = parameter, kolom = corner (FL, FR, RL, RR), refresh 10x per detik.
- **Graph**: jendela bergulir 60 detik, sumbu X = jam, pilih parameter dan corner.

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

## Format paket (harus sama dengan `zone_data` di firmware, 23 byte, little-endian)
 0 current_ref u16 (A + 40) | 2 current_motor u16 (A + 100) x10
 4 erpm u32 (ERPM + 30000) | 8 duty_cycle u16 (duty + 1) x1000
10 current_in u16 (A + 100) x10 | 12 v_in u16 (V x10)
14 steer_in u16 (sudut target x100 + 13498) | 16 steer u16 (sudut aktual x100 + 13498)
18 steer_stretch u8 | 19 steer_speed u8 | 20 steer_current u8
21 steer_temp u8 | 22 telemetry_valid u8 (bit 0..3: stretch, speed, current, temp)
