#pragma once
#include <QString>
#include <QtGlobal>

// Hasil parsing satu paket telemetri ZCU (format versi 2, 26 byte)
struct ZcuTelemetry {
    QString mac;
    quint8 seq = 0;

    double currentA = 0;     // arus motor (A)
    int    erpm = 0;
    double duty = 0;         // 0..1
    double tempFet = 0;      // C
    double tempMotor = 0;    // C
    double vin = 0;          // V
    double currentIn = 0;    // A

    int    steerRaw = 0;
    double steerDeg = 0;
    double steerLoadPct = 0;
    int    steerTemp = 0;
    double steerVolt = 0;

    int cmdSteer = 0;
    int cmdMotor = 0;

    qint64 timeMs = 0;       // waktu diterima (ms sejak epoch)
};
