#pragma once
#include <QMap>
#include <QString>
#include <QStringList>
#include "RawEthMonitor.h"

// Status yang dibagi antar jendela
class AppState
{
public:
    static AppState &instance();

    RawEthMonitor monitor;
    QString adapterName;                 // nama adapter untuk pcap
    QString adapterDisplay;              // teks tampilan
    QMap<QString, QString> macs;         // nama device -> MAC

    // Urutan kolom: FL, FR, RL, RR
    static const QStringList &cornerKeys();     // "ZCU (FL)", ...
    static const QStringList &cornerLabels();   // "FL", ...
    static const QStringList &defaultMacs();

    // MAC corner ke-i (dari Setup, atau default)
    QString cornerMac(int i) const;
    QString vcuMac() const;

    void load();    // baca dari QSettings
    void save();    // simpan ke QSettings

private:
    AppState() = default;
};
