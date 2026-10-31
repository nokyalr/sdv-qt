#pragma once
#include <QList>
#include <QPointF>
#include <QScrollArea>
#include <QWidget>

class QCheckBox;
class QComboBox;
class QLineSeries;
class QTimer;
class QValueAxis;
class QChartView;
class QChart;

// Satu baris panel grafik (parameter selector + chart)
class GraphPanel : public QWidget
{
    Q_OBJECT
public:
    explicit GraphPanel(QWidget *parent = nullptr);

    // Dipanggil oleh GraphPage tiap 50 ms
    void tick(qint64 nowMs, bool paused);
    void clearData();

private:
    void onParamChanged();
    void applyTheme();
    void changeEvent(QEvent *e) override;

    static constexpr int kWindowMs  = 60'000;
    static constexpr int kOfflineMs = 500;
    static constexpr double kScrollSpeed = 0.5;

    QComboBox   *m_paramBox  = nullptr;
    QCheckBox   *m_corner[4] = {};
    QCheckBox   *m_autoChk   = nullptr;

    QChart      *m_chart     = nullptr;
    QChartView  *m_view      = nullptr;
    QLineSeries *m_series[4] = {};
    QList<QPointF> m_points[4];
    QValueAxis  *m_axisX     = nullptr;
    QValueAxis  *m_axisY     = nullptr;
    int m_remoteParamOffset = -1;
    qint64 m_lastTickMs = 0;
};

// Halaman Graph: dua panel (kiri dan kanan), sumbu X = detik relatif
class GraphPage : public QWidget
{
    Q_OBJECT
public:
    explicit GraphPage(QWidget *parent = nullptr);

private:
    void tick();

    QCheckBox  *m_pauseChk = nullptr;
    GraphPanel *m_panel[2] = {};
    QTimer     *m_timer    = nullptr;
};
