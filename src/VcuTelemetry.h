#pragma once

#include <QtGlobal>

// Payload logger_data_t VCU (little-endian, packed, 16 bytes):
// raw_steer u32, raw_speed u32, map_steer float, map_speed float.
struct VcuTelemetry {
    quint32 steerRemoteRaw = 0;    // pulse steering (us)
    quint32 throttleRemoteRaw = 0; // pulse throttle (us)
    double steerRemoteMap = 0;    // nilai map_steer dari VCU
    double throttleRemoteMap = 0; // nilai map_speed 0..200 dari VCU
    qint64 timeMs = 0;   // waktu diterima (ms sejak epoch)

    double throttleRemoteSigned() const { return throttleRemoteMap - 100.0; }
};
