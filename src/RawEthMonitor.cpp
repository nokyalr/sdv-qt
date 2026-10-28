#include "RawEthMonitor.h"

#include <QDateTime>
#include <QMutexLocker>
#include <QtEndian>
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

// Layout payload sdv_cornerv3 (mulai byte 14, packed, little-endian, 23 byte):
//  0 current_ref   u16  (current_ref_A + 40)
//  2 current_motor u16  (motor_current_A + 100) * 10
//  4 erpm          u32  erpm + 30000
//  8 duty_cycle    u16  (duty + 1.0) * 1000
// 10 current_in    u16  (batt_current_A + 100) * 10
// 12 v_in          u16  voltage_V * 10
// 14 steer_in      u16  target_deg100 + 13498
// 16 steer         u16  actual_deg100 + 13498
// 18 steer_stretch u8
// 19 steer_speed   u8
// 20 steer_current u8
// 21 steer_temp    u8
// 22 telemetry_valid u8
bool parse(const unsigned char *d, int len, const QString &mac, ZcuTelemetry &t)
{
    if (len < 14 + 23) return false;
    if (d[12] != 0x88 || d[13] != 0xB5) return false;
    const unsigned char *p = d + 14;

    auto u16 = [&](int o) { return qFromLittleEndian<quint16>(p + o); };
    auto u32 = [&](int o) { return qFromLittleEndian<quint32>(p + o); };

    t.mac = mac;
    t.currentRef   = u16(0) - 40.0;                      // offset 40
    t.currentMotor = u16(2) / 10.0 - 100.0;              // (val/10) - 100
    t.erpm         = static_cast<int>(u32(4)) - 30000;   // offset 30000
    t.duty         = u16(8) / 1000.0 - 1.0;              // (val/1000) - 1
    t.currentIn    = u16(10) / 10.0 - 100.0;             // (val/10) - 100
    t.vin          = u16(12) / 10.0;                      // val/10
    t.steerIn      = (static_cast<int>(u16(14)) - 13498) / 100.0;  // deg
    t.steerActual  = (static_cast<int>(u16(16)) - 13498) / 100.0;  // deg
    t.steerStretch = p[18];
    t.steerSpeed   = p[19];
    t.steerCurrent = p[20];
    t.steerTemp    = p[21];
    t.steerTelemetryValid = p[22];
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

    ZcuTelemetry t;
    const bool ok = parse(data, len, src, t);

    QMutexLocker lock(&m_mutex);
    m_seen[src] = now;
    if (ok && src != m_vcuMac) {
        m_latest[src] = t;
    } else if (src == m_vcuMac && len >= 14 + 8) {
        VcuTelemetry vcu;
        vcu.steerRemoteRaw = qFromLittleEndian<quint32>(data + 14);
        vcu.throttleRemoteRaw = qFromLittleEndian<quint32>(data + 18);
        vcu.steerRemoteMap = mapVcuSteer(vcu.steerRemoteRaw);
        vcu.throttleRemoteMap = mapVcuThrottle(vcu.throttleRemoteRaw);
        vcu.timeMs = now;
        m_latestVcu[src] = vcu;
    }
}

void RawEthMonitor::clearSeen()
{
    QMutexLocker lock(&m_mutex);
    m_seen.clear();
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
