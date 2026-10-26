#pragma once
#include <QMainWindow>

class QLabel;
class QPushButton;
class QStackedWidget;
#include <QList>

// Jendela utama: menu kiri + area halaman
class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);

signals:
    void setupRequested();   // sinyal balik ke SetupWindow

private slots:
    void refreshStatus();

private:
    QStackedWidget *m_pages      = nullptr;
    QList<QPushButton*> m_navButtons;  // tombol navigasi sidebar

    // Status bar labels
    QLabel *m_lblAdapter         = nullptr;
    QLabel *m_lblOnline          = nullptr;
    QLabel *m_lblDtc             = nullptr;
    QLabel *m_lblLastUpdate      = nullptr;
};
