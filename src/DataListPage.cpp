#include "DataListPage.h"
#include "AppState.h"
#include "Params.h"

#include <QBrush>
#include <QDateTime>
#include <QHeaderView>
#include <QLabel>
#include <QTableWidget>
#include <QTimer>
#include <QVBoxLayout>
#include <QStringList>

namespace {
constexpr int kOfflineMs = 500;   // corner dianggap Offline kalau tak ada paket selama 500 ms
}

DataListPage::DataListPage(QWidget *parent) : QWidget(parent)
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(16, 16, 16, 16);

    auto *title = new QLabel("Monitoring Parameter");
    QFont f = title->font(); f.setPointSize(f.pointSize() + 5); f.setBold(true);
    title->setFont(f);
    layout->addWidget(title);

    const auto &params = parameters();
    constexpr int remoteRows = 4;
    m_table = new QTableWidget(params.size() + 1 + remoteRows, 7);
    m_table->setHorizontalHeaderLabels(
        {"Parameter", "Unit", "ZCU FL", "ZCU FR", "ZCU RL", "ZCU RR", "Remote"});
    m_table->verticalHeader()->setVisible(false);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setSelectionMode(QAbstractItemView::NoSelection);
    m_table->setAlternatingRowColors(true);
    m_table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);

    auto addLabel = [&](int row, const QString &name, const QString &unit) {
        m_table->setItem(row, 0, new QTableWidgetItem(name));
        m_table->setItem(row, 1, new QTableWidgetItem(unit));
        for (int c = 2; c < 7; ++c) {
            auto *it = new QTableWidgetItem("-");
            it->setTextAlignment(Qt::AlignCenter);
            m_table->setItem(row, c, it);
        }
    };
    addLabel(0, "Status", "");
    for (int i = 0; i < params.size(); ++i) addLabel(i + 1, params[i].name, params[i].unit);

    const QStringList remoteNames = {
        "Steer Remote Raw", "Steer Remote Map",
        "Throttle Remote Raw", "Throttle Remote Map"
    };
    for (int i = 0; i < remoteRows; ++i)
        addLabel(params.size() + 1 + i, remoteNames.at(i), "");

    layout->addWidget(m_table, 1);

    // UI di-refresh 10x per detik
    m_timer = new QTimer(this);
    connect(m_timer, &QTimer::timeout, this, &DataListPage::refresh);
    m_timer->start(100);
}

void DataListPage::refresh()
{
    auto &app = AppState::instance();
    const auto &params = parameters();
    const qint64 now = QDateTime::currentMSecsSinceEpoch();

    for (int c = 0; c < 4; ++c) {
        const auto t = app.monitor.latest(app.cornerMac(c));
        const bool online = t && (now - t->timeMs) < kOfflineMs;

        auto *st = m_table->item(0, 2 + c);
        st->setText(online ? "Online" : "Offline");
        st->setForeground(QBrush(online ? QColor("#2E9E55") : QColor("#E5595B")));

        for (int i = 0; i < params.size(); ++i) {
            const bool available = online && params[i].valid(*t);
            m_table->item(i + 1, 2 + c)->setText(
                available ? QString::number(params[i].value(*t), 'f', params[i].decimals) : "-");
        }
    }

    const auto vcu = app.monitor.latestVcu(app.vcuMac());
    const bool vcuOnline = vcu && (now - vcu->timeMs) < kOfflineMs;
    const QStringList values = vcuOnline
                                   ? QStringList{
                                         QString::number(vcu->steerRemoteRaw),
                                         QString::number(vcu->steerRemoteMap, 'f', 2),
                                         QString::number(vcu->throttleRemoteRaw),
                                         QString::number(vcu->throttleRemoteMap, 'f', 2)}
                                   : QStringList{"-", "-", "-", "-"};
    for (int i = 0; i < values.size(); ++i) {
        auto *value = m_table->item(params.size() + 1 + i, 6);
        value->setText(values.at(i));
        value->setForeground(QBrush(vcuOnline ? QColor("#2E9E55") : QColor("#E5595B")));
    }
}
