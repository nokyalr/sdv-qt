#pragma once
#include <QWidget>
#include <QVector>

class QComboBox;
class QLabel;
class QLineEdit;
class QPushButton;

// Jendela "Setup Configuration"
class SetupWindow : public QWidget
{
    Q_OBJECT
public:
    explicit SetupWindow(QWidget *parent = nullptr);

signals:
    void continueRequested();

private:
    enum class Status { NotTested, Testing, Connected, Disconnect };

    void refreshAdapters();
    void resetToDefault();
    void testConnection();
    void saveAndContinue();
    void setStatus(int row, Status s);

    struct Row {
        QString name;
        QString defaultMac;
        QLineEdit *mac = nullptr;
        QLabel *dot = nullptr;
        QLabel *text = nullptr;
    };

    QComboBox *m_adapterBox = nullptr;
    QPushButton *m_testBtn = nullptr;
    QVector<Row> m_rows;
};
