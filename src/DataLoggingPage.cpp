#include "DataLoggingPage.h"
#include "AppState.h"
#include "Params.h"

#include <QBrush>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QFont>
#include <QFrame>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QStandardPaths>
#include <QTableWidget>
#include <QTimer>
#include <QVBoxLayout>

#ifdef Q_OS_UNIX
#include <pwd.h>
#endif

namespace {
constexpr int  kOfflineMs  = 500;
constexpr int  kTickMs     = 100;   // tulis CSV tiap 100 ms (10 Hz)
constexpr int  kPreviewFontSize = 16;
constexpr int  kPreviewRowHeight = 42;

QString defaultLogDirectory()
{
#ifdef Q_OS_UNIX
    // When started with sudo, Qt sees /root as HOME. Use the invoking user's home.
    const QByteArray sudoUser = qgetenv("SUDO_USER");
    if (!sudoUser.isEmpty()) {
        if (const passwd *user = getpwnam(sudoUser.constData()); user && user->pw_dir) {
            const QDir userHome(QString::fromLocal8Bit(user->pw_dir));
            const QString documents = userHome.filePath("Documents");
            return userHome.exists("Documents") ? documents : userHome.path();
        }
    }
#endif
    return QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
}
}

DataLoggingPage::DataLoggingPage(QWidget *parent) : QWidget(parent)
{
    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(16, 16, 16, 16);
    root->setSpacing(10);

    // ---- Group: File output ----
    auto *fileGroup = new QGroupBox("Output File");
    auto *fileLayout = new QHBoxLayout(fileGroup);
    fileLayout->setSpacing(6);

    m_pathEdit = new QLineEdit;
    m_pathEdit->setPlaceholderText("Pilih lokasi file CSV...");
    // Default: Dokumen / log_<tanggal>.csv
    const QString fileName = "sdv-qt_log_"
                             + QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss")
                             + ".csv";
    const QString defPath = QDir(defaultLogDirectory()).filePath(fileName);
    m_pathEdit->setText(defPath);

    m_browseBtn = new QPushButton("Browse");
    m_browseBtn->setFixedWidth(100);

    m_startBtn = new QPushButton("▶ Start");
    m_startBtn->setFixedWidth(100);
    m_startBtn->setStyleSheet(
        "QPushButton{background:#2E9E55;color:white;border-radius:4px;padding:5px 10px;font-weight:bold;}"
        "QPushButton:hover{background:#25804A;}"
        "QPushButton:pressed{background:#1E6A3D;}");

    fileLayout->addWidget(new QLabel("Path:"));
    fileLayout->addWidget(m_pathEdit, 1);
    fileLayout->addWidget(m_browseBtn);
    fileLayout->addWidget(m_startBtn);
    root->addWidget(fileGroup);

    // ---- Group: Status ----
    auto *statusGroup = new QGroupBox("Status");
    auto *statusGrid  = new QGridLayout(statusGroup);
    statusGrid->setSpacing(8);

    auto makeStatusLabel = [](const QString &val) {
        auto *l = new QLabel(val);
        QFont f = l->font();
        f.setPointSize(f.pointSize() + 1);
        l->setFont(f);
        return l;
    };

    m_lblStatus   = makeStatusLabel("⏸  Idle");
    m_lblRows     = makeStatusLabel("Rows: 0");
    m_lblDuration = makeStatusLabel("Duration: 0 s");
    m_lblSize     = makeStatusLabel("File: 0 KB");

    statusGrid->addWidget(new QLabel("Status:"),    0, 0);
    statusGrid->addWidget(m_lblStatus,              0, 1);
    statusGrid->addWidget(new QLabel("Rows:"),      0, 2);
    statusGrid->addWidget(m_lblRows,                0, 3);
    statusGrid->addWidget(new QLabel("Duration:"),  1, 0);
    statusGrid->addWidget(m_lblDuration,            1, 1);
    statusGrid->addWidget(new QLabel("File size:"), 1, 2);
    statusGrid->addWidget(m_lblSize,                1, 3);
    statusGrid->setColumnStretch(1, 1);
    statusGrid->setColumnStretch(3, 1);
    root->addWidget(statusGroup);

    // ---- Group: Live preview ----
    auto *previewGroup = new QGroupBox("Live Preview");
    previewGroup->setFont(QFont("Segoe UI", kPreviewFontSize));
    auto *previewLayout = new QVBoxLayout(previewGroup);

    const auto &params = parameters();
    const auto &corners = AppState::cornerLabels();

    // Baris = corner/VCU, kolom = device + parameter atau data VCU.
    m_preview = new QTableWidget(5, params.size() + 5);
    m_preview->setFont(QFont("Segoe UI", kPreviewFontSize));
    QStringList headers;
    headers << "Corner";
    for (const Param &p : params)
        headers << (p.unit.isEmpty() ? p.name : QString("%1 (%2)").arg(p.name, p.unit));
    headers << "Steer Remote Raw (us)" << "Throttle Remote Raw (us)"
            << "Steer Remote Map" << "Throttle Remote Map";
    m_preview->setHorizontalHeaderLabels(headers);
    m_preview->verticalHeader()->setVisible(false);
    m_preview->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_preview->setSelectionMode(QAbstractItemView::NoSelection);
    m_preview->setAlternatingRowColors(true);
    m_preview->setStyleSheet(
        "QScrollBar:vertical { width: 36px; }"
        "QScrollBar::handle:vertical { min-height: 48px; }"
        "QScrollBar:horizontal { height: 36px; }"
        "QScrollBar::handle:horizontal { min-width: 48px; }");
    m_preview->horizontalHeader()->setFont(QFont("Segoe UI", kPreviewFontSize));
    m_preview->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    m_preview->horizontalHeader()->setStretchLastSection(true);

    for (int r = 0; r < 5; ++r) {
        m_preview->setItem(r, 0, new QTableWidgetItem(r < 4 ? corners.at(r) : "Remote"));
        m_preview->item(r, 0)->setTextAlignment(Qt::AlignCenter);
        for (int c = 1; c < m_preview->columnCount(); ++c) {
            auto *it = new QTableWidgetItem("-");
            it->setTextAlignment(Qt::AlignCenter);
            m_preview->setItem(r, c, it);
        }
        m_preview->setRowHeight(r, kPreviewRowHeight);
    }
    previewLayout->addWidget(m_preview);
    root->addWidget(previewGroup, 1);

    // ---- Konekaaaaaaasi ----
    connect(m_browseBtn, &QPushButton::clicked, this, &DataLoggingPage::browseFile);
    connect(m_startBtn,  &QPushButton::clicked, this, &DataLoggingPage::toggleLogging);

    // Timer: update preview (dan tulis CSV saat logging)
    m_timer = new QTimer(this);
    connect(m_timer, &QTimer::timeout, this, &DataLoggingPage::tick);
    m_timer->start(kTickMs);
}

// ----------------------------------------------------------------
void DataLoggingPage::browseFile()
{
    const QString path = QFileDialog::getSaveFileName(
        this, "Simpan File Log", m_pathEdit->text(),
        "CSV Files (*.csv);;All Files (*)");
    if (!path.isEmpty())
        m_pathEdit->setText(path);
}

// ----------------------------------------------------------------
void DataLoggingPage::toggleLogging()
{
    if (!m_logging) {
        // --- Mulai logging ---
        m_filePath = m_pathEdit->text().trimmed();
        if (m_filePath.isEmpty()) {
            QMessageBox::warning(this, "Data Logging", "Pilih file output terlebih dahulu.");
            return;
        }
        m_rowCount = 0;
        m_startMs  = QDateTime::currentMSecsSinceEpoch();
        m_logging  = true;
        writeHeader();

        m_pathEdit->setEnabled(false);
        m_browseBtn->setEnabled(false);
        m_startBtn->setText("■  Stop");
        m_startBtn->setStyleSheet(
            "QPushButton{background:#E5595B;color:white;border-radius:4px;padding:5px 10px;font-weight:bold;}"
            "QPushButton:hover{background:#C04040;}"
            "QPushButton:pressed{background:#A03030;}");
    } else {
        // --- Hentikan logging ---
        m_logging = false;
        m_pathEdit->setEnabled(true);
        m_browseBtn->setEnabled(true);
        m_startBtn->setText("▶  Start");
        m_startBtn->setStyleSheet(
            "QPushButton{background:#2E9E55;color:white;border-radius:4px;padding:5px 10px;font-weight:bold;}"
            "QPushButton:hover{background:#25804A;}"
            "QPushButton:pressed{background:#1E6A3D;}");
        updateStatus();
    }
}

// ----------------------------------------------------------------
void DataLoggingPage::writeHeader()
{
    QFile f(m_filePath);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
        QMessageBox::critical(this, "Data Logging",
                              "Tidak bisa membuat file:\n" + m_filePath);
        m_logging = false;
        return;
    }
    QTextStream ts(&f);

    // Header: Timestamp_ms, Corner, Param1, Param2, ...
    ts << "Timestamp_ms,Corner";
    for (const Param &p : parameters())
        ts << "," << (p.unit.isEmpty() ? p.name : p.name + " (" + p.unit + ")");
    ts << ",Steer Remote Raw (us),Throttle Remote Raw (us)"
       << ",Steer Remote Map,Throttle Remote Map";
    ts << "\n";
}

// ----------------------------------------------------------------
void DataLoggingPage::writeRow(qint64 nowMs)
{
    QFile f(m_filePath);
    if (!f.open(QIODevice::Append | QIODevice::Text)) return;
    QTextStream ts(&f);

    auto &app = AppState::instance();
    const auto &params  = parameters();
    const auto &corners = AppState::cornerLabels();

    for (int i = 0; i < 4; ++i) {
        const auto t      = app.monitor.latest(app.cornerMac(i));
        const bool online = t && (nowMs - t->timeMs) < kOfflineMs;
        if (!online) continue;   // skip corner offline

        ts << nowMs << "," << corners.at(i);
        for (const Param &p : params)
            ts << "," << QString::number(p.value(*t), 'f', p.decimals);
        ts << ",,,,";
        ts << "\n";
        ++m_rowCount;
    }

    const auto vcu = app.monitor.latestVcu(app.vcuMac());
    if (vcu && (nowMs - vcu->timeMs) < kOfflineMs) {
        ts << nowMs << ",Remote";
        for (const Param &p : params)
            ts << ",";
        ts << "," << vcu->steerRemoteRaw << "," << vcu->throttleRemoteRaw
           << "," << QString::number(vcu->steerRemoteMap, 'f', 2)
           << "," << QString::number(vcu->throttleRemoteSigned(), 'f', 2) << "\n";
        ++m_rowCount;
    }
}

// ----------------------------------------------------------------
void DataLoggingPage::tick()
{
    const qint64 nowMs = QDateTime::currentMSecsSinceEpoch();

    // --- Update live preview ---
    auto &app = AppState::instance();
    const auto &params = parameters();

    for (int r = 0; r < 4; ++r) {
        const auto t      = app.monitor.latest(app.cornerMac(r));
        const bool online = t && (nowMs - t->timeMs) < kOfflineMs;

        const QBrush okBrush(online ? QColor("#2E9E55") : QColor("#E5595B"));
        m_preview->item(r, 0)->setForeground(okBrush);

        for (int c = 0; c < params.size(); ++c) {
            m_preview->item(r, c + 1)->setText(
                online ? QString::number(params[c].value(*t), 'f', params[c].decimals) : "-");
        }
    }

    const auto vcu = app.monitor.latestVcu(app.vcuMac());
    const bool vcuOnline = vcu && (nowMs - vcu->timeMs) < kOfflineMs;
    m_preview->item(4, 0)->setForeground(
        QBrush(vcuOnline ? QColor("#2E9E55") : QColor("#E5595B")));
    for (int c = 1; c <= params.size(); ++c)
        m_preview->item(4, c)->setText("-");
    m_preview->item(4, params.size() + 1)->setText(
        vcuOnline ? QString::number(vcu->steerRemoteRaw) : "-");
    m_preview->item(4, params.size() + 2)->setText(
        vcuOnline ? QString::number(vcu->throttleRemoteRaw) : "-");
    m_preview->item(4, params.size() + 3)->setText(
        vcuOnline ? QString::number(vcu->steerRemoteMap, 'f', 2) : "-");
    m_preview->item(4, params.size() + 4)->setText(
        vcuOnline ? QString::number(vcu->throttleRemoteSigned(), 'f', 2) : "-");

    // --- Tulis ke CSV jika sedang logging ---
    if (m_logging) {
        writeRow(nowMs);
        updateStatus();
    }
}

// ----------------------------------------------------------------
void DataLoggingPage::updateStatus()
{
    if (m_logging) {
        m_lblStatus->setText("🔴  Recording...");
        m_lblStatus->setStyleSheet("color:#E5595B; font-weight:bold;");
    } else {
        m_lblStatus->setText(m_rowCount > 0 ? "✅  Stopped" : "⏸  Idle");
        m_lblStatus->setStyleSheet("");
    }

    m_lblRows->setText(QString::number(m_rowCount));

    const qint64 elapsedSec = m_logging
                                  ? (QDateTime::currentMSecsSinceEpoch() - m_startMs) / 1000
                                  : (m_startMs > 0
                                         ? (QDateTime::currentMSecsSinceEpoch() - m_startMs) / 1000
                                         : 0);
    m_lblDuration->setText(QString("%1 s").arg(elapsedSec));

    if (!m_filePath.isEmpty() && QFileInfo::exists(m_filePath)) {
        const double kb = QFileInfo(m_filePath).size() / 1024.0;
        m_lblSize->setText(QString("%1 KB").arg(kb, 0, 'f', 1));
    } else {
        m_lblSize->setText("0 KB");
    }
}
