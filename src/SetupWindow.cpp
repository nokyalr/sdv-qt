#include "SetupWindow.h"
#include "AppState.h"

#include <QComboBox>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QTimer>
#include <QToolButton>
#include <QVBoxLayout>

SetupWindow::SetupWindow(QWidget *parent) : QWidget(parent)
{
    setWindowTitle("Setup Configuration");
    setFixedSize(440, 400);

    AppState::instance().load();

    // Daftar device: 4 ZCU + VCU
    const auto &keys = AppState::cornerKeys();
    const auto &defs = AppState::defaultMacs();
    m_rows = {
        {keys[0], defs[0]},                      // FL
        {keys[1], defs[1]},                      // FR
        {keys[2], defs[2]},                      // RL
        {keys[3], defs[3]},                      // RR
        {"VCU",   "00:14:2D:E3:6B:2D"},
    };

    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(20, 14, 20, 16);

    // --- Baris adapter ---
    auto *adapterRow = new QHBoxLayout;
    adapterRow->addWidget(new QLabel("Adapter"));
    m_adapterBox = new QComboBox;
    m_adapterBox->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    adapterRow->addWidget(m_adapterBox, 1);
    auto *refreshBtn = new QToolButton;
    refreshBtn->setText("⟳");
    refreshBtn->setToolTip("Refresh daftar adapter");
    adapterRow->addWidget(refreshBtn);
    root->addLayout(adapterRow);
    root->addSpacing(8);

    // --- Tabel device ---
    auto *grid = new QGridLayout;
    grid->setColumnStretch(1, 1);
    auto header = [](const QString &t) {
        auto *l = new QLabel(t);
        l->setAlignment(Qt::AlignCenter);
        QFont f = l->font(); f.setBold(true); l->setFont(f);
        return l;
    };
    grid->addWidget(header("Device"), 0, 0);
    grid->addWidget(header("MAC Address"), 0, 1);
    grid->addWidget(header("Status"), 0, 2, 1, 2);

    for (int i = 0; i < m_rows.size(); ++i) {
        Row &r = m_rows[i];
        auto *name = new QLabel(r.name);
        name->setAlignment(Qt::AlignCenter);
        r.mac = new QLineEdit;
        r.mac->setInputMask(">HH:HH:HH:HH:HH:HH;_");
        r.mac->setAlignment(Qt::AlignCenter);
        r.dot = new QLabel;
        r.dot->setFixedSize(14, 14);
        r.text = new QLabel;
        r.text->setMinimumWidth(80);

        grid->addWidget(name, i + 1, 0);
        grid->addWidget(r.mac, i + 1, 1);
        grid->addWidget(r.dot, i + 1, 2);
        grid->addWidget(r.text, i + 1, 3);
        grid->setRowMinimumHeight(i + 1, 38);
    }
    root->addLayout(grid);
    root->addStretch(1);

    // --- Tombol bawah ---
    auto *btnRow = new QHBoxLayout;
    auto *resetBtn = new QPushButton("Reset to Default");
    m_testBtn = new QPushButton("Test Connection");
    auto *saveBtn = new QPushButton("Save && Continue");
    btnRow->addWidget(resetBtn);
    btnRow->addWidget(m_testBtn);
    btnRow->addWidget(saveBtn);
    root->addLayout(btnRow);

    connect(refreshBtn, &QToolButton::clicked, this, &SetupWindow::refreshAdapters);
    connect(resetBtn, &QPushButton::clicked, this, &SetupWindow::resetToDefault);
    connect(m_testBtn, &QPushButton::clicked, this, &SetupWindow::testConnection);
    connect(saveBtn, &QPushButton::clicked, this, &SetupWindow::saveAndContinue);

    // Isi MAC dari pengaturan tersimpan, lalu status awal
    for (int i = 0; i < m_rows.size(); ++i) {
        const Row &r = m_rows[i];
        m_rows[i].mac->setText(AppState::instance().macs.value(r.name, r.defaultMac));
        setStatus(i, Status::NotTested);
    }
    refreshAdapters();
}

void SetupWindow::refreshAdapters()
{
    const QString previous = m_adapterBox->currentData().toString().isEmpty()
                                 ? AppState::instance().adapterName
                                 : m_adapterBox->currentData().toString();
    m_adapterBox->clear();

    QString err;
    const auto list = RawEthMonitor::listAdapters(&err);
    if (!err.isEmpty())
        QMessageBox::warning(this, "Adapter",
                             "Tidak bisa membaca daftar adapter.\n"
                             "Pastikan libpcap/Npcap terpasang dan program punya izin (root / CAP_NET_RAW).\n\n" + err);

    int select = -1;
    for (int i = 0; i < list.size(); ++i) {
        m_adapterBox->addItem(list[i].display, list[i].name);
        if (list[i].name == previous) select = i;
        else if (select < 0 && list[i].display.contains("eth", Qt::CaseInsensitive)) select = i;
    }
    if (select >= 0) m_adapterBox->setCurrentIndex(select);
    m_testBtn->setEnabled(m_adapterBox->count() > 0);
}

void SetupWindow::resetToDefault()
{
    for (int i = 0; i < m_rows.size(); ++i) {
        m_rows[i].mac->setText(m_rows[i].defaultMac);
        setStatus(i, Status::NotTested);
    }
}

void SetupWindow::testConnection()
{
    auto &app = AppState::instance();
    QString err;
    if (!app.monitor.start(m_adapterBox->currentData().toString(), &err)) {
        QMessageBox::critical(this, "Test Connection", "Gagal membuka adapter jaringan:\n" + err);
        return;
    }

    app.monitor.clearSeen();
    for (int i = 0; i < m_rows.size(); ++i) setStatus(i, Status::Testing);
    m_testBtn->setEnabled(false);

    // STM kirim tiap 20 ms, 1,5 detik cukup
    QTimer::singleShot(1500, this, [this] {
        for (int i = 0; i < m_rows.size(); ++i)
            setStatus(i, AppState::instance().monitor.wasSeen(m_rows[i].mac->text())
                             ? Status::Connected : Status::Disconnect);
        m_testBtn->setEnabled(true);
    });
}

void SetupWindow::saveAndContinue()
{
    auto &app = AppState::instance();
    app.adapterName = m_adapterBox->currentData().toString();
    app.adapterDisplay = m_adapterBox->currentText();
    app.macs.clear();
    for (const Row &r : m_rows) app.macs[r.name] = r.mac->text();
    app.save();
    emit continueRequested();      // monitor tidak dihentikan, dipakai terus oleh MainWindow
}

void SetupWindow::setStatus(int row, Status s)
{
    QString color, text;
    switch (s) {
    case Status::NotTested: color = "#D0D0D0"; text = "Not Tested"; break;
    case Status::Testing:   color = "#F5C542"; text = "Testing..."; break;
    case Status::Connected: color = "#7BE495"; text = "Connected";  break;
    case Status::Disconnect:color = "#E5595B"; text = "Disconnect"; break;
    }
    m_rows[row].dot->setStyleSheet(QString("background:%1; border-radius:7px;").arg(color));
    m_rows[row].text->setText(text);
}
