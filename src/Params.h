#pragma once
#include <QString>
#include <QVector>
#include <functional>
#include "ZcuTelemetry.h"

// Definisi parameter, dipakai bersama oleh Data List dan Graph.
// Menambah parameter baru = menambah satu baris di Params.h.
struct Param {
    QString name;
    QString unit;
    int decimals;
    std::function<double(const ZcuTelemetry &)> value;
    std::function<bool(const ZcuTelemetry &)> valid;
};

inline const QVector<Param> &parameters()
{
    static const QVector<Param> p = {
        {"Current Ref",     "A",   1, [](const ZcuTelemetry &t) { return t.currentRef; }, [](const ZcuTelemetry &) { return true; }},
        {"Motor Current",   "A",   1, [](const ZcuTelemetry &t) { return t.currentMotor; }, [](const ZcuTelemetry &) { return true; }},
        {"ERPM",            "rpm", 0, [](const ZcuTelemetry &t) { return double(t.erpm); }, [](const ZcuTelemetry &) { return true; }},
        {"Duty Cycle",      "%",   1, [](const ZcuTelemetry &t) { return t.duty * 100.0; }, [](const ZcuTelemetry &) { return true; }},
        {"Battery Current", "A",   1, [](const ZcuTelemetry &t) { return t.currentIn; }, [](const ZcuTelemetry &) { return true; }},
        {"Battery Voltage", "V",   1, [](const ZcuTelemetry &t) { return t.vin; }, [](const ZcuTelemetry &) { return true; }},
        {"Steer Target",    "°",   2, [](const ZcuTelemetry &t) { return t.steerIn; }, [](const ZcuTelemetry &) { return true; }},
        {"Steer Actual",    "°",   2, [](const ZcuTelemetry &t) { return t.steerActual; }, [](const ZcuTelemetry &) { return true; }},
        {"Servo Stretch",   "",    0, [](const ZcuTelemetry &t) { return t.steerStretch; }, [](const ZcuTelemetry &t) { return t.steerTelemetryValid & 0x01; }},
        {"Servo Speed",     "",    0, [](const ZcuTelemetry &t) { return t.steerSpeed; }, [](const ZcuTelemetry &t) { return t.steerTelemetryValid & 0x02; }},
        {"Servo Current",   "",    0, [](const ZcuTelemetry &t) { return t.steerCurrent; }, [](const ZcuTelemetry &t) { return t.steerTelemetryValid & 0x04; }},
        {"Servo Temp Raw",  "",    0, [](const ZcuTelemetry &t) { return t.steerTemp; }, [](const ZcuTelemetry &t) { return t.steerTelemetryValid & 0x08; }},
    };
    return p;
}
