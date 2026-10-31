#pragma once
#include <QString>
#include <QtGlobal>

// Hasil parsing satu paket telemetri ZCU (56 byte payload, little-endian).
// Payload berisi 14 float IEEE-754 dalam urutan yang sama dengan zone_data:
// v_in, current_in, current_motor, erpm, temp_fet, temp_motor, duty_cycle,
// wh_used, wh_charged, steer, stretch, speed, current, temp.
struct ZcuTelemetry {
    QString mac;

    double vin = 0;          // V
    double currentIn = 0;    // A
    double currentMotor = 0; // A
    double erpm = 0;         // electrical RPM
    double tempFet = 0;      // deg C
    double tempMotor = 0;    // deg C
    double duty = 0;         // percent
    double whUsed = 0;       // Wh
    double whCharged = 0;    // Wh
    double steer = 0;        // wheel angle, left negative and right positive
    double stretch = 0;      // percent
    double speed = 0;        // percent
    double current = 0;      // percent, negative means reverse direction
    double temp = 0;         // heat level percent, 0 cold and 100 hot

    qint64 timeMs = 0;       // waktu diterima (ms sejak epoch)
};
