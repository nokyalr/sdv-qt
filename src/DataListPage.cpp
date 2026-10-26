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
    m_table = new QTableWidget(params.size() + 1, 6);       // +1 untuk baris Status
    m_table->setHorizontalHeaderLabels({"Parameter", "Unit", "ZCU FL", "ZCU FR", "ZCU RL", "ZCU RR"});
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
        for (int c = 2; c < 6; ++c) {
            auto *it = new QTableWidgetItem("-");
            it->setTextAlignment(Qt::AlignCenter);
            m_table->setItem(row, c, it);
        }
    };
    addLabel(0, "Status", "");
    for (int i = 0; i < params.size(); ++i) addLabel(i + 1, params[i].name, params[i].unit);

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
            m_table->item(i + 1, 2 + c)->setText(
                online ? QString::number(params[i].value(*t), 'f', params[i].decimals) : "-");
        }
    }
}
