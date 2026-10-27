#pragma once
#include <QWidget>

class QLabel;
class QLineEdit;
class QPushButton;
class QTableWidget;
class QTimer;

// Halaman Data Logging: rekam semua parameter ZCU dan data pulse VCU ke file CSV
class DataLoggingPage : public QWidget
{
    Q_OBJECT
public:
    explicit DataLoggingPage(QWidget *parent = nullptr);

private slots:
    void browseFile();
    void toggleLogging();
    void tick();

private:
    void updateStatus();
    void writeHeader();
    void writeRow(qint64 nowMs);

    // Kontrol atas
    QLineEdit   *m_pathEdit    = nullptr;
    QPushButton *m_browseBtn   = nullptr;
    QPushButton *m_startBtn    = nullptr;

    // Status
    QLabel *m_lblStatus        = nullptr;   // Idle / Recording...
    QLabel *m_lblRows          = nullptr;   // Rows: 0
    QLabel *m_lblDuration      = nullptr;   // Duration: 0s
    QLabel *m_lblSize          = nullptr;   // File: 0 KB

    // Tabel live preview (baris = corner, kolom = parameter)
    QTableWidget *m_preview    = nullptr;

    // Logging state
    QTimer  *m_timer           = nullptr;
    bool     m_logging         = false;
    qint64   m_startMs         = 0;
    quint64  m_rowCount        = 0;
    QString  m_filePath;
};
