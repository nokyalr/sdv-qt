#pragma once
#include <QHash>
#include <QList>
#include <QMutex>
#include <QString>
#include <atomic>
#include <optional>
#include <thread>
#include "ZcuTelemetry.h"

struct AdapterInfo {
    QString name;      // nama untuk pcap (mis. "eth0" atau "\\Device\\NPF_{...}")
    QString display;   // teks di dropdown
};

// Sniffer raw Ethernet (EtherType 0x88B5) memakai libpcap, berjalan di thread sendiri.
class RawEthMonitor
{
public:
    RawEthMonitor() = default;
    ~RawEthMonitor();

    static QList<AdapterInfo> listAdapters(QString *error = nullptr);

    bool start(const QString &adapterName, QString *error = nullptr);
    void stop();
    QString currentAdapter() const { return m_adapter; }

    void clearSeen();
    bool wasSeen(const QString &mac) const;
    std::optional<ZcuTelemetry> latest(const QString &mac) const;

private:
    void handlePacket(const unsigned char *data, int len);

    std::thread m_thread;
    std::atomic<bool> m_running{false};
    QString m_adapter;

    mutable QMutex m_mutex;
    QHash<QString, qint64> m_seen;
    QHash<QString, ZcuTelemetry> m_latest;
};
