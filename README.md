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

## Format paket (harus sama dengan zone_data di firmware, versi 2, 26 byte, little-endian)
 0 version u8 (=2) | 1 seq u8 | 2 motor_current i16 x10 | 4 erpm i32 | 8 duty i16 x1000
10 temp_fet i16 x10 | 12 temp_motor i16 x10 | 14 v_in i16 x10 | 16 current_in i16 x10
18 steer_raw u16 | 20 steer_load u16 | 22 steer_temp u8 | 23 steer_volt u8 x10 | 24 cmd_steer u8 | 25 cmd_motor u8
