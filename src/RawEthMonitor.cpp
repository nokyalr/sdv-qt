#include "RawEthMonitor.h"

#include <QDateTime>
#include <QMutexLocker>
#include <QtEndian>
#include <algorithm>
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

// Layout payload (mulai byte 14, packed, little-endian) - sama dengan zone_data di firmware:
//  0 version u8 | 1 seq u8 | 2 motor_current i16 (x10) | 4 erpm i32 | 8 duty i16 (x1000)
// 10 temp_fet i16 (x10) | 12 temp_motor i16 (x10) | 14 v_in i16 (x10) | 16 current_in i16 (x10)
// 18 steer_raw u16 | 20 steer_load u16 | 22 steer_temp u8 | 23 steer_volt u8 (x10)
// 24 cmd_steer u8 | 25 cmd_motor u8
bool parse(const unsigned char *d, int len, const QString &mac, ZcuTelemetry &t)
{
    if (len < 14 + 26) return false;
    if (d[12] != 0x88 || d[13] != 0xB5) return false;
    const unsigned char *p = d + 14;
    if (p[0] != 2) return false;                         // versi paket

    auto i16 = [&](int o) { return qFromLittleEndian<qint16>(p + o); };
    auto u16 = [&](int o) { return qFromLittleEndian<quint16>(p + o); };
    auto i32 = [&](int o) { return qFromLittleEndian<qint32>(p + o); };

    t.mac = mac;
    t.seq = p[1];
    t.currentA = i16(2) / 10.0;
    t.erpm = i32(4);
    t.duty = i16(8) / 1000.0;
    t.tempFet = i16(10) / 10.0;
    t.tempMotor = i16(12) / 10.0;
    t.vin = i16(14) / 10.0;
    t.currentIn = i16(16) / 10.0;

    t.steerRaw = u16(18);
    // Sama dengan dxl_position_to_degree() di firmware: 204..785 -> 0..180 derajat
    t.steerDeg = std::clamp((t.steerRaw - 204) * 180.0 / (785 - 204), 0.0, 180.0);

    int loadRaw = u16(20);                               // bit 0-9 besar, bit 10 arah
    t.steerLoadPct = (loadRaw & 0x3FF) * 0.097 * (((loadRaw >> 10) & 1) == 0 ? -1.0 : 1.0);
    t.steerTemp = p[22];
    t.steerVolt = p[23] / 10.0;
    t.cmdSteer = p[24];
    t.cmdMotor = p[25];
    t.timeMs = QDateTime::currentMSecsSinceEpoch();
    return true;
}

} // namespace

RawEthMonitor::~RawEthMonitor() { stop(); }

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
    if (ok) m_latest[src] = t;
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
