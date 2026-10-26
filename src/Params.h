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
};

inline const QVector<Param> &parameters()
{
    static const QVector<Param> p = {
        {"Motor Current",   "A",   1, [](const ZcuTelemetry &t) { return t.currentA; }},
        {"ERPM",            "rpm", 0, [](const ZcuTelemetry &t) { return double(t.erpm); }},
        {"Duty Cycle",      "%",   1, [](const ZcuTelemetry &t) { return t.duty * 100.0; }},
        {"FET Temp",        "°C",  1, [](const ZcuTelemetry &t) { return t.tempFet; }},
        {"Motor Temp",      "°C",  1, [](const ZcuTelemetry &t) { return t.tempMotor; }},
        {"Battery Voltage", "V",   1, [](const ZcuTelemetry &t) { return t.vin; }},
        {"Battery Current", "A",   1, [](const ZcuTelemetry &t) { return t.currentIn; }},
        {"Steer Angle",     "°",   1, [](const ZcuTelemetry &t) { return t.steerDeg; }},
        {"Steer (raw)",     "",    0, [](const ZcuTelemetry &t) { return double(t.steerRaw); }},
        {"Steer Load",      "%",   1, [](const ZcuTelemetry &t) { return t.steerLoadPct; }},
        {"Servo Temp",      "°C",  0, [](const ZcuTelemetry &t) { return double(t.steerTemp); }},
        {"Servo Voltage",   "V",   1, [](const ZcuTelemetry &t) { return t.steerVolt; }},
        {"Cmd Steer (VCU)", "",    0, [](const ZcuTelemetry &t) { return double(t.cmdSteer); }},
        {"Cmd Motor (VCU)", "",    0, [](const ZcuTelemetry &t) { return double(t.cmdMotor); }},
        {"Packet Seq",      "",    0, [](const ZcuTelemetry &t) { return double(t.seq); }},
    };
    return p;
}
