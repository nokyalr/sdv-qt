#include "RawEthMonitor.h"

#include <QDateTime>
#include <QMutexLocker>
#include <QtEndian>
#include <cstring>
#include <pcap.h>

namespace {

QString macToString(const unsigned char *p)
{
    return QString("%1:%2:%3:%4:%5:%6")
        .arg(p[0], 2, 16, QLatin1Char('0'))
        .arg(p[1], 2, 16, QLatin1Char('0'))
        .arg(p[2], 2, 16, QLatin1Char('0'))
        .arg(p[3], 2, 16, QLatin1Char('0'))
        .arg(p[4], 2, 16, QLatin1Char('0'))
        .arg(p[5], 2, 16, QLatin1Char('0'))
        .toUpper();
}

constexpr int kZonePayloadSize = 14 * int(sizeof(float));

bool parse(const unsigned char *d, int len, const QString &mac, ZcuTelemetry &t)
{
    static_assert(sizeof(float) == 4, "zone_data requires 32-bit floats");
    if (len < 14 + kZonePayloadSize) return false;
    if (d[12] != 0x88 || d[13] != 0xB5) return false;
    const unsigned char *p = d + 14;

    auto f32 = [&](int o) {
        float value;
        std::memcpy(&value, p + o, sizeof(value));
        return static_cast<double>(value);
    };

    t.mac = mac;
    t.vin          = f32(0);
    t.currentIn    = f32(4);
    t.currentMotor = f32(8);
    t.erpm         = f32(12);
    t.tempFet      = f32(16);
    t.tempMotor    = f32(20);
    t.duty         = f32(24);
    t.whUsed       = f32(28);
    t.whCharged    = f32(32);
    t.steer        = f32(36);
    t.stretch      = f32(40);
    t.speed        = f32(44);
    t.current      = f32(48);
    t.temp         = f32(52);
    t.timeMs = QDateTime::currentMSecsSinceEpoch();
    return true;
}

} // namespace

RawEthMonitor::~RawEthMonitor() { stop(); }

void RawEthMonitor::setVcuMac(const QString &mac)
{
    QMutexLocker lock(&m_mutex);
    m_vcuMac = mac.trimmed().toUpper();
}

QList<AdapterInfo> RawEthMonitor::listAdapters(QString *error)
{
    QList<AdapterInfo> out;
    char errbuf[PCAP_ERRBUF_SIZE] = {0};
    pcap_if_t *devs = nullptr;

    if (pcap_findalldevs(&devs, errbuf) != 0) {
        if (error) *error = QString::fromLocal8Bit(errbuf);
        return out;
    }
    for (pcap_if_t *d = devs; d; d = d->next) {
        if (d->flags & PCAP_IF_LOOPBACK) continue;
        QString name = QString::fromLocal8Bit(d->name);
        QString desc = d->description ? QString::fromLocal8Bit(d->description) : QString();
        out.append({name, desc.isEmpty() ? name : desc});
    }
    pcap_freealldevs(devs);
    return out;
}

bool RawEthMonitor::start(const QString &adapterName, QString *error)
{
    if (m_running && m_adapter == adapterName) return true;   // sudah berjalan di adapter ini
    stop();

    char errbuf[PCAP_ERRBUF_SIZE] = {0};
    pcap_t *h = pcap_open_live(adapterName.toLocal8Bit().constData(), 2048, 1, 100, errbuf);
    if (!h) {
        if (error) *error = QString::fromLocal8Bit(errbuf);
        return false;
    }

    bpf_program fp;
    if (pcap_compile(h, &fp, "ether proto 0x88b5", 1, PCAP_NETMASK_UNKNOWN) == 0) {
        pcap_setfilter(h, &fp);
        pcap_freecode(&fp);
    }

    m_adapter = adapterName;
    m_running = true;
    m_thread = std::thread([this, h] {
        pcap_pkthdr *hdr = nullptr;
        const unsigned char *data = nullptr;
        while (m_running) {
            int r = pcap_next_ex(h, &hdr, &data);
            if (r == 1) handlePacket(data, int(hdr->caplen));
            else if (r < 0) break;                            // error / adapter hilang
        }
        pcap_close(h);
    });
    return true;
}

void RawEthMonitor::stop()
{
    m_running = false;
    if (m_thread.joinable()) m_thread.join();
    m_adapter.clear();
}

void RawEthMonitor::handlePacket(const unsigned char *data, int len)
{
    if (len < 14) return;
    const QString src = macToString(data + 6);
    const qint64 now = QDateTime::currentMSecsSinceEpoch();

    QMutexLocker lock(&m_mutex);
    if (src == m_vcuMac && len >= 14 + 16) {
        VcuTelemetry vcu;
        vcu.steerRemoteRaw = qFromLittleEndian<quint32>(data + 14);
        vcu.throttleRemoteRaw = qFromLittleEndian<quint32>(data + 18);
        float steerMap;
        float throttleMap;
        std::memcpy(&steerMap, data + 22, sizeof(steerMap));
        std::memcpy(&throttleMap, data + 26, sizeof(throttleMap));
        vcu.steerRemoteMap = static_cast<double>(steerMap);
        vcu.throttleRemoteMap = static_cast<double>(throttleMap);
        vcu.timeMs = now;
        m_seen[src] = now;
        m_latestVcu[src] = vcu;
        return;
    }

    ZcuTelemetry t;
    if (parse(data, len, src, t)) {
        m_seen[src] = now;
        m_latest[src] = t;
    }
}

void RawEthMonitor::clearSeen()
{
    QMutexLocker lock(&m_mutex);
    m_seen.clear();
    m_latest.clear();
    m_latestVcu.clear();
}

bool RawEthMonitor::wasSeen(const QString &mac) const
{
    QMutexLocker lock(&m_mutex);
    return m_seen.contains(mac.trimmed().toUpper());
}

std::optional<ZcuTelemetry> RawEthMonitor::latest(const QString &mac) const
{
    QMutexLocker lock(&m_mutex);
    auto it = m_latest.constFind(mac.trimmed().toUpper());
    if (it == m_latest.constEnd()) return std::nullopt;
    return *it;
}

std::optional<VcuTelemetry> RawEthMonitor::latestVcu(const QString &mac) const
{
    QMutexLocker lock(&m_mutex);
    auto it = m_latestVcu.constFind(mac.trimmed().toUpper());
    if (it == m_latestVcu.constEnd()) return std::nullopt;
    return *it;
}
