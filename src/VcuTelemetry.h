#pragma once

#include <QtGlobal>

// Payload logger_data_t dari VCU (packed, little-endian).
struct VcuTelemetry {
    quint32 steerRemoteRaw = 0;    // pulse steering (us)
    quint32 throttleRemoteRaw = 0; // pulse throttle (us)
    double steerRemoteMap = 0;
    double throttleRemoteMap = 0;
    qint64 timeMs = 0;   // waktu diterima (ms sejak epoch)
};
