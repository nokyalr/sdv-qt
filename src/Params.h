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
        {"Input Voltage",   "V",   2, [](const ZcuTelemetry &t) { return t.vin; }, [](const ZcuTelemetry &) { return true; }},
        {"Input Current",   "A",   2, [](const ZcuTelemetry &t) { return t.currentIn; }, [](const ZcuTelemetry &) { return true; }},
        {"Motor Current",   "A",   2, [](const ZcuTelemetry &t) { return t.currentMotor; }, [](const ZcuTelemetry &) { return true; }},
        {"ERPM",            "rpm", 0, [](const ZcuTelemetry &t) { return t.erpm; }, [](const ZcuTelemetry &) { return true; }},
        {"FET Temperature", "°C",  1, [](const ZcuTelemetry &t) { return t.tempFet; }, [](const ZcuTelemetry &) { return true; }},
        {"Motor Temperature","°C", 1, [](const ZcuTelemetry &t) { return t.tempMotor; }, [](const ZcuTelemetry &) { return true; }},
        {"Duty Cycle",      "%",   1, [](const ZcuTelemetry &t) { return t.duty; }, [](const ZcuTelemetry &) { return true; }},
        {"Wh Used",         "Wh",  2, [](const ZcuTelemetry &t) { return t.whUsed; }, [](const ZcuTelemetry &) { return true; }},
        {"Wh Charged",      "Wh",  2, [](const ZcuTelemetry &t) { return t.whCharged; }, [](const ZcuTelemetry &) { return true; }},
        {"Steer",           "°",   2, [](const ZcuTelemetry &t) { return t.steer; }, [](const ZcuTelemetry &) { return true; }},
        {"Servo Stretch",       "%",       1, [](const ZcuTelemetry &t) { return t.stretch; }, [](const ZcuTelemetry &) { return true; }},
        {"Servo Speed",         "%",       1, [](const ZcuTelemetry &t) { return t.speed; }, [](const ZcuTelemetry &) { return true; }},
        {"Servo Current Level", "% level", 1, [](const ZcuTelemetry &t) { return t.current; }, [](const ZcuTelemetry &) { return true; }},
        {"Servo Heat Level",    "% heat",  1, [](const ZcuTelemetry &t) { return t.temp; }, [](const ZcuTelemetry &) { return true; }},
    };
    return p;
}
