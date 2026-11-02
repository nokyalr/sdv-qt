#include "DataListPage.h"
#include "AppState.h"
#include "Params.h"

#include <QBrush>
#include <QDateTime>
#include <QFont>
#include <QHeaderView>
#include <QLabel>
#include <QTableWidget>
#include <QTimer>
#include <QVBoxLayout>
#include <QStringList>

namespace {
constexpr int kOfflineMs = 500;   // corner dianggap Offline kalau tak ada paket selama 500 ms
constexpr int kMotorRows = 9;
constexpr int kServoRows = 5;
constexpr int kRemoteRows = 4;
constexpr int kStatusRow = 2;
constexpr int kMotorStartRow = 3;
constexpr int kServoStartRow = kMotorStartRow + kMotorRows;
constexpr int kVcuTitleRow = kServoStartRow + kServoRows;
constexpr int kRemoteStartRow = kVcuTitleRow + 1;
constexpr int kListFontSize = 16;
constexpr int kListRowHeight = 42;
}

DataListPage::DataListPage(QWidget *parent) : QWidget(parent)
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(16, 16, 16, 16);

    const auto &params = parameters();
    constexpr int zoneTitleRow = 0;
    constexpr int headerRow = 1;

    m_table = new QTableWidget(kRemoteStartRow + kRemoteRows, 7);
    m_table->setFont(QFont("Segoe UI", kListFontSize));
    m_table->setHorizontalHeaderLabels(
        {"", "Parameter", "FRONT LEFT", "FRONT RIGHT", "REAR LEFT", "REAR RIGHT", "Value"});
    m_table->verticalHeader()->setVisible(false);
    m_table->horizontalHeader()->setVisible(false);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setSelectionMode(QAbstractItemView::NoSelection);
    m_table->setShowGrid(true);
    m_table->setWordWrap(false);
    m_table->setStyleSheet(
        "QTableWidget { background: #d9d9d9; gridline-color: #b5b5b5; }"
        "QTableWidget::item { padding: 4px; }"
        "QScrollBar:vertical { width: 36px; }"
        "QScrollBar::handle:vertical { min-height: 40px; }");
    m_table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Fixed);
    m_table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_table->setColumnWidth(0, 110);
    m_table->setColumnWidth(1, 150);
    m_table->setTextElideMode(Qt::ElideNone);

    auto addCell = [&](int row, int column, const QString &text) {
        auto *item = new QTableWidgetItem(text);
        item->setTextAlignment(Qt::AlignCenter);
        m_table->setItem(row, column, item);
    };
    auto addSectionTitle = [&](int row, const QString &text, const QColor &color) {
        addCell(row, 0, text);
        m_table->setSpan(row, 0, 1, 7);
        auto *item = m_table->item(row, 0);
        item->setBackground(color);
        item->setFont(QFont("Segoe UI", kListFontSize, QFont::DemiBold));
    };
    auto addDataRow = [&](int row, const QString &name, const QString &unit) {
        addCell(row, 1, name);
        addCell(row, 6, unit);
        for (int c = 2; c < 6; ++c) addCell(row, c, "-");
    };

    addSectionTitle(zoneTitleRow, "ZONE CONTROL UNIT", QColor("#9edfe3"));
    for (int c = 0; c < 7; ++c) addCell(headerRow, c, m_table->horizontalHeaderItem(c)->text());
    addCell(kStatusRow, 1, "Status");
    for (int c = 2; c < 6; ++c) addCell(kStatusRow, c, "Offline");
    addCell(kStatusRow, 6, "Online/Offline");

    const QStringList motorNames = {
        "Voltage in", "Current in", "Current Motor", "Duty Cycle", "ERPM",
        "Temp FET", "Temp Motor", "Wh used", "Wh charged"
    };
    const QStringList motorUnits = {
        "Volt", "Ampere", "Ampere", "%", "rpm", "°C", "°C", "Wh", "Wh"
    };
    for (int i = 0; i < kMotorRows; ++i)
        addDataRow(kMotorStartRow + i, motorNames.at(i), motorUnits.at(i));

    const QStringList servoNames = {"Steer", "Stretch", "Speed", "Current", "Temp"};
    const QStringList servoUnits = {"°", "%", "%", "% level", "% heat"};
    for (int i = 0; i < kServoRows; ++i)
        addDataRow(kServoStartRow + i, servoNames.at(i), servoUnits.at(i));

    addCell(kMotorStartRow, 0, "Motor");
    m_table->setSpan(kMotorStartRow, 0, kMotorRows, 1);
    addCell(kServoStartRow, 0, "Servo");
    m_table->setSpan(kServoStartRow, 0, kServoRows, 1);

    addSectionTitle(kVcuTitleRow, "VEHICLE CONTROL UNIT", QColor("#69dfaa"));
    const QStringList remoteNames = {"Steer raw", "Throttle raw", "Steer map", "Throttle map"};
    const QStringList remoteUnits = {"us", "us", "°", "%"};
    for (int i = 0; i < kRemoteRows; ++i) {
        const int row = kRemoteStartRow + i;
        addCell(row, 1, remoteNames.at(i));
        addCell(row, 6, remoteUnits.at(i));
        addCell(row, 2, "");
        m_table->setSpan(row, 2, 1, 4);
    }
    addCell(kRemoteStartRow, 0, "Remote");
    m_table->setSpan(kRemoteStartRow, 0, kRemoteRows, 1);

    for (int row = 0; row < m_table->rowCount(); ++row) m_table->setRowHeight(row, kListRowHeight);

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

        auto *st = m_table->item(kStatusRow, 2 + c);
        st->setText(online ? "Online" : "Offline");
        st->setForeground(QBrush(online ? QColor("#2E9E55") : QColor("#E5595B")));

        for (int i = 0; i < params.size(); ++i) {
            const bool available = online && params[i].valid(*t);
            const int row = i < kMotorRows ? kMotorStartRow + i : kServoStartRow + (i - kMotorRows);
            m_table->item(row, 2 + c)->setText(
                available ? QString::number(params[i].value(*t), 'f', params[i].decimals) : "-");
        }
    }

    const auto vcu = app.monitor.latestVcu(app.vcuMac());
    const bool vcuOnline = vcu && (now - vcu->timeMs) < kOfflineMs;
    const QStringList values = vcuOnline
                                   ? QStringList{
                                         QString::number(vcu->steerRemoteRaw),
                                         QString::number(vcu->throttleRemoteRaw),
                                         QString::number(vcu->steerRemoteMap, 'f', 2),
                                         QString::number(vcu->throttleRemoteSigned(), 'f', 2)}
                                   : QStringList{"-", "-", "-", "-"};
    for (int i = 0; i < values.size(); ++i) {
        auto *value = m_table->item(kRemoteStartRow + i, 2);
        value->setText(values.at(i));
        value->setForeground(QBrush(vcuOnline ? QColor("#2E9E55") : QColor("#E5595B")));
    }
}
