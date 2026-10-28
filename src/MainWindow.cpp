#include "MainWindow.h"
#include "AppState.h"
#include "DataListPage.h"
#include "DataLoggingPage.h"
#include "GraphPage.h"

#include <QAction>
#include <QDateTime>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QMenuBar>
#include <QMessageBox>
#include <QPushButton>
#include <QStackedWidget>
#include <QStatusBar>
#include <QTimer>
#include <QVBoxLayout>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent)
{
    setWindowTitle("sdv-qt");
    resize(980, 540);
    setMinimumSize(700, 420);

    auto &app = AppState::instance();

    // Pastikan monitor berjalan di adapter yang dipilih
    QString err;
    if (!app.adapterName.isEmpty() && !app.monitor.start(app.adapterName, &err))
        QMessageBox::critical(this, "Adapter", "Gagal membuka adapter:\n" + err);

    // ----------------------------------------------------------------
    // Central widget
    // ----------------------------------------------------------------
    auto *central    = new QWidget;
    auto *root       = new QHBoxLayout(central);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    // ----------------------------------------------------------------
    // Menu bar atas: Setup
    // ----------------------------------------------------------------
    auto *actSetup = new QAction("Setup", this);
    actSetup->setToolTip("Kembali ke jendela konfigurasi");
    connect(actSetup, &QAction::triggered, this, &MainWindow::setupRequested);
    menuBar()->addAction(actSetup);

    auto *actSettings = new QAction("Settings", this);
    actSettings->setToolTip("Pengaturan aplikasi");
    actSettings->setEnabled(false);  // placeholder – belum diimplementasi
    menuBar()->addAction(actSettings);

    // ----------------------------------------------------------------
    // Sidebar kiri
    // ----------------------------------------------------------------
    auto *menu = new QWidget;
    menu->setFixedWidth(160);
    menu->setObjectName("sideMenu");
    menu->setAttribute(Qt::WA_StyledBackground, true);
    menu->setStyleSheet("#sideMenu{background:palette(alternate-base);}");

    auto *menuLayout = new QVBoxLayout(menu);
    menuLayout->setContentsMargins(8, 6, 8, 6);
    menuLayout->setSpacing(4);

    // ----------------------------------------------------------------
    // Halaman
    // ----------------------------------------------------------------
    m_pages = new QStackedWidget;
    m_pages->addWidget(new DataListPage);    // 0 – Data List
    m_pages->addWidget(new GraphPage);       // 1 – Graph
    m_pages->addWidget(new DataLoggingPage); // 2 – Data Logging

    // ----------------------------------------------------------------
    // Tombol navigasi
    // ----------------------------------------------------------------
    static const QString kActiveColor  = "#d4f576";
    static const QString kActiveBorder = "#000000"; 

    auto addMenu = [&](const QString &text, int page) {
        auto *b = new QPushButton(text);
        b->setMinimumHeight(36);
        b->setObjectName(QString("navBtn_%1").arg(m_navButtons.size()));
        if (page >= 0) {
            connect(b, &QPushButton::clicked, this, [this, page] {
                m_pages->setCurrentIndex(page);
            });
        } else {
            b->setEnabled(false);
        }
        menuLayout->addWidget(b);
        m_navButtons.append(b);
    };

    addMenu("Data List",    0); 
    addMenu("Graph",       -1);
    addMenu("Data Logging", 2);
    addMenu("Active Test",  -1);
    addMenu("Configuration", -1);
    addMenu("DTC",          -1);
    menuLayout->addStretch(1);

    // Fungsi highlight tombol aktif
    auto updateNavHighlight = [this]() {
        const int cur = m_pages->currentIndex();
        for (int i = 0; i < m_navButtons.size(); ++i) {
            QPushButton *b = m_navButtons[i];
            if (i == cur) {
                b->setStyleSheet(QString(
                    "QPushButton {"
                    "  background: %1;"
                    "  color: #1a1a1a;"
                    "  border: 1px solid %2;"
                    "  border-radius: 6px;"
                    "  font-weight: bold;"
                    "  padding: 4px 8px;"
                    "}"
                    "QPushButton:hover { background: %1; }"
                ).arg(kActiveColor, kActiveBorder));
            } else {
                b->setStyleSheet("");  // kembali ke default stylesheet
            }
        }
    };

    // Update saat halaman berubah
    connect(m_pages, &QStackedWidget::currentChanged, this, [updateNavHighlight](int) {
        updateNavHighlight();
    });
    updateNavHighlight();  // inisialisasi awal

    root->addWidget(menu);
    root->addWidget(m_pages, 1);
    setCentralWidget(central);

    // ----------------------------------------------------------------
    // Status bar bawah
    // ----------------------------------------------------------------
    m_lblAdapter = new QLabel;
    m_lblAdapter->setText(app.adapterDisplay.isEmpty() ? "(belum dipilih)" : app.adapterDisplay);
    m_lblAdapter->setToolTip(app.adapterDisplay);
    m_lblAdapter->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);

    // Pemisah vertikal
    auto makeSep = [] {
        auto *f = new QFrame;
        f->setFrameShape(QFrame::VLine);
        f->setFrameShadow(QFrame::Sunken);
        return f;
    };

    m_lblOnline     = new QLabel("0 / 4 ONLINE");
    m_lblDtc        = new QLabel("0 DTC");
    m_lblLastUpdate = new QLabel("Last Update: --:--:--");

    // Letakkan di status bar menggunakan widget kustom agar bisa diberi separator
    auto *sb = statusBar();
    sb->addWidget(m_lblAdapter, 1);      // stretch=1 → dorong sisanya ke kanan
    sb->addPermanentWidget(makeSep());
    sb->addPermanentWidget(m_lblOnline);
    sb->addPermanentWidget(makeSep());
    sb->addPermanentWidget(m_lblDtc);
    sb->addPermanentWidget(makeSep());
    sb->addPermanentWidget(m_lblLastUpdate);

    // ----------------------------------------------------------------
    // Timer refresh status bar (100 ms)
    // ----------------------------------------------------------------
    auto *timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &MainWindow::refreshStatus);
    timer->start(100);
}

// ----------------------------------------------------------------
// refreshStatus – perbarui indikator bawah tiap 100 ms
// ----------------------------------------------------------------
void MainWindow::refreshStatus()
{
    auto &app = AppState::instance();
    const qint64 now = QDateTime::currentMSecsSinceEpoch();
    constexpr int kOfflineMs = 500;

    int onlineCount = 0;
    qint64 lastMs   = 0;

    for (int i = 0; i < 4; ++i) {
        const auto t = app.monitor.latest(app.cornerMac(i));
        if (t && (now - t->timeMs) < kOfflineMs) {
            ++onlineCount;
            if (t->timeMs > lastMs) lastMs = t->timeMs;
        }
    }

    m_lblOnline->setText(QString("%1 / 4 ONLINE").arg(onlineCount));
    m_lblDtc->setText("0 DTC"); 

    if (lastMs > 0) {
        const QString ts = QDateTime::fromMSecsSinceEpoch(lastMs).toString("hh:mm:ss");
        m_lblLastUpdate->setText("Last Update: " + ts);
    } else {
        m_lblLastUpdate->setText("Last Update: --:--:--");
    }
}
