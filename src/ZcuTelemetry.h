#pragma once
#include <QString>
#include <QtGlobal>

// Hasil parsing satu paket telemetri ZCU (format sdv_cornerv3, 23 byte payload)
//
// Layout zone_data (packed, little-endian, EtherType 0x88B5):
//  Offset  Field           Type      Encoding
//  0       current_ref     u16       (current_ref_A + 40)             -> A
//  2       current_motor   u16       (motor_current + 100) * 10       -> A
//  4       erpm            u32       erpm + 30000                     -> RPM
//  8       duty_cycle      u16       (duty + 1.0) * 1000              -> 0..1
// 10       current_in      u16       (batt_current + 100) * 10        -> A
// 12       v_in            u16       voltage * 10                     -> V
// 14       steer_in        u16       target_deg100 + 13498            -> deg*100
// 16       steer           u16       actual_deg100 + 13498            -> deg*100
// 18       steer_stretch  u8        1..127
// 19       steer_speed    u8        1..127
// 20       steer_current  u8        0..127 (64..127 arah sebaliknya)
// 21       steer_temp     u8        0..127 (127 dingin .. 0 panas)
// 22       telemetry_valid u8       bit 0..3 sesuai empat field di atas
struct ZcuTelemetry {
    QString mac;

    double currentRef = 0;      // arus referensi ke VESC (A)
    double currentMotor = 0;    // arus motor aktual (A)
    int    erpm = 0;            // electrical RPM
    double duty = 0;            // duty cycle 0..1
    double currentIn = 0;       // arus input baterai (A)
    double vin = 0;             // tegangan input (V)
    double steerIn = 0;         // target sudut steer (deg, ±134.98)
    double steerActual = 0;     // sudut steer aktual (deg, ±134.98)
    double steerStretch = 0;
    double steerSpeed = 0;
    double steerCurrent = 0;
    double steerTemp = 0;
    quint8 steerTelemetryValid = 0;

    qint64 timeMs = 0;          // waktu diterima (ms sejak epoch)
};
