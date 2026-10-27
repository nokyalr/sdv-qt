#include "AppState.h"
#include <QSettings>

AppState &AppState::instance()
{
    static AppState s;
    return s;
}

const QStringList &AppState::cornerKeys()
{
    static const QStringList k = {"ZCU (FL)", "ZCU (FR)", "ZCU (RL)", "ZCU (RR)"};
    return k;
}

const QStringList &AppState::cornerLabels()
{
    static const QStringList k = {"FL", "FR", "RL", "RR"};
    return k;
}

const QStringList &AppState::defaultMacs()
{
    // MAC STM = 00:33:CE:00:00:<ID_MACAddr_CORNER> (lihat config_sdv.h)
    static const QStringList m = {"00:33:CE:00:00:01", "00:33:CE:00:00:02",
                                  "00:33:CE:00:00:03", "00:33:CE:00:00:04"};
    return m;
}

QString AppState::cornerMac(int i) const
{
    return macs.value(cornerKeys().at(i), defaultMacs().at(i)).trimmed().toUpper();
}

QString AppState::vcuMac() const
{
    return macs.value("VCU", "00:14:2D:E3:6B:2D").trimmed().toUpper();
}

void AppState::load()
{
    QSettings s("sdv-qt", "sdv-qt");
    adapterName = s.value("adapter").toString();
    s.beginGroup("macs");
    for (const QString &k : s.childKeys()) macs[k] = s.value(k).toString();
    s.endGroup();
}

void AppState::save()
{
    QSettings s("sdv-qt", "sdv-qt");
    s.setValue("adapter", adapterName);
    s.beginGroup("macs");
    for (auto it = macs.cbegin(); it != macs.cend(); ++it) s.setValue(it.key(), it.value());
    s.endGroup();
}
