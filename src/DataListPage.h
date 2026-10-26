#pragma once
#include <QWidget>

class QTableWidget;
class QTimer;

// Halaman Data List: baris = parameter, kolom = corner (FL, FR, RL, RR)
class DataListPage : public QWidget
{
    Q_OBJECT
public:
    explicit DataListPage(QWidget *parent = nullptr);

private:
    void refresh();

    QTableWidget *m_table = nullptr;
    QTimer *m_timer = nullptr;
};
