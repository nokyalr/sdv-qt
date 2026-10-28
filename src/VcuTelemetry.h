#pragma once

#include <QtGlobal>

// Field raw pertama dari payload logger_data_t VCU (little-endian).
struct VcuTelemetry {
    quint32 steerRemoteRaw = 0;    // pulse steering (us)
    quint32 throttleRemoteRaw = 0; // pulse throttle (us)
    double steerRemoteMap = 0;
    double throttleRemoteMap = 0;
    qint64 timeMs = 0;   // waktu diterima (ms sejak epoch)
};

inline double interpolateVcuInput(double value, double inMin, double inMid, double inMax,
                                  double outMin, double outMid, double outMax)
{
    if (value <= inMid)
        return (value - inMin) * (outMid - outMin) / (inMid - inMin) + outMin;
    return (value - inMid) * (outMax - outMid) / (inMax - inMid) + outMid;
}

inline double mapVcuSteer(double value)
{
    return interpolateVcuInput(value, 1140, 1630, 2004, 450, 0, -450);
}

inline double mapVcuThrottle(double value)
{
    return interpolateVcuInput(value, 1003, 1521, 2002, 0, 100, 200);
}
