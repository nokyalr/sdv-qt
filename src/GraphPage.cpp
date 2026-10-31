#include "GraphPage.h"
#include "AppState.h"
#include "Params.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDateTime>
#include <QEvent>
#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QScrollArea>
#include <QStringList>
#include <QTimer>
#include <QVBoxLayout>
#include <QWheelEvent>
#include <QtCharts/QChart>
#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>
#include <QtCharts/QValueAxis>
#include <algorithm>
#include <cmath>
#include <limits>

// ============================================================
//  InteractiveChartView – zoom via scroll wheel
// ============================================================
class InteractiveChartView : public QChartView
{
public:
    explicit InteractiveChartView(QChart *chart, QWidget *parent = nullptr)
        : QChartView(chart, parent) {}

protected:
    void wheelEvent(QWheelEvent *e) override {
        const double factor = e->angleDelta().y() > 0 ? 1.0 / 1.15 : 1.15;
        chart()->zoom(factor);
        e->accept();
    }
};

// ============================================================
//  GraphPanel
// ============================================================
GraphPanel::GraphPanel(QWidget *parent) : QWidget(parent)
{
    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(6, 4, 6, 4);
    root->setSpacing(3);

    // ---- Baris 1 kontrol: Dropdown Parameter & Auto scale ----
    auto *row1 = new QHBoxLayout;
    row1->setSpacing(6);

    auto *paramLbl = new QLabel("Param:");
    QFont lf = paramLbl->font();
    lf.setBold(true);
    paramLbl->setFont(lf);
    row1->addWidget(paramLbl);

    m_paramBox = new QComboBox;
    m_paramBox->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    m_paramBox->setMinimumWidth(90);
    for (const Param &p : parameters()) m_paramBox->addItem(p.name);
    m_remoteParamOffset = parameters().size();
    m_paramBox->addItems({"Steer Remote Raw", "Steer Remote Map",
                          "Throttle Remote Raw", "Throttle Remote Map"});
    row1->addWidget(m_paramBox, 1);

    m_autoChk = new QCheckBox("Auto scale");
    m_autoChk->setChecked(true);
    row1->addWidget(m_autoChk);

    root->addLayout(row1);

    // ---- Baris 2 kontrol: Checkbox Corner (FL / FR / RL / RR) ----
    auto *row2 = new QHBoxLayout;
    row2->setSpacing(8);

    static const QString kCornerColors[4] = {
        "#4472C4",  // FL – biru
        "#ED7D31",  // FR – oranye
        "#70AD47",  // RL – hijau
        "#FF0000",  // RR – merah
    };

    const QStringList &labels = AppState::cornerLabels();
    for (int i = 0; i < 4; ++i) {
        m_corner[i] = new QCheckBox(labels.at(i));
        m_corner[i]->setChecked(true);
        m_corner[i]->setStyleSheet(QString("QCheckBox { font-weight: bold; color: %1; }").arg(kCornerColors[i]));
        row2->addWidget(m_corner[i]);
    }
    row2->addStretch(1);

    root->addLayout(row2);

    // ---- Area grafik: ChartView ----
    auto *chartRow = new QHBoxLayout;
    chartRow->setSpacing(0);

    // ---- Chart ----
    m_chart = new QChart;
    m_chart->legend()->setAlignment(Qt::AlignLeft);   // legenda di dalam kiri bawah
    m_chart->legend()->setBackgroundVisible(true);
    m_chart->setMargins(QMargins(0, 4, 4, 0));
    m_chart->setBackgroundRoundness(0);

    // Sumbu X: detik relatif (0 = sekarang, -60 = 60 detik lalu)
    m_axisX = new QValueAxis;
    m_axisX->setTitleText("Time (sec)");
    m_axisX->setRange(-60, 0);
    m_axisX->setTickCount(13);           // -60, -55, -50, ... 0
    m_axisX->setLabelFormat("%g");
    m_axisX->setMinorTickCount(4);

    m_axisY = new QValueAxis;
    m_axisY->setLabelFormat("%g");
    m_axisY->setRange(-1, 1);

    m_chart->addAxis(m_axisX, Qt::AlignBottom);
    m_chart->addAxis(m_axisY, Qt::AlignLeft);

    // Warna seri: biru, oranye, hijau, merah (mirip screenshot)
    static const QColor kColors[4] = {
        QColor("#4472C4"),  // FL – biru
        QColor("#ED7D31"),  // FR – oranye
        QColor("#70AD47"),  // RL – hijau
        QColor("#FF0000"),  // RR – merah
    };

    for (int i = 0; i < 4; ++i) {
        m_series[i] = new QLineSeries;
        m_series[i]->setName(labels.at(i));
        QPen pen(kColors[i]);
        pen.setWidth(2);
        m_series[i]->setPen(pen);
        m_chart->addSeries(m_series[i]);
        m_series[i]->attachAxis(m_axisX);
        m_series[i]->attachAxis(m_axisY);
    }

    m_view = new InteractiveChartView(m_chart);
    m_view->setRenderHint(QPainter::Antialiasing);
    m_view->setMinimumHeight(140);
    // Rubber band aktif saat auto scale OFF (agar bisa zoom drag)
    connect(m_autoChk, &QCheckBox::checkStateChanged, this, [this](int state) {
        m_view->setRubberBand(state == Qt::Unchecked
                                  ? QChartView::RectangleRubberBand
                                  : QChartView::NoRubberBand);
    });

    chartRow->addWidget(m_view, 1);
    root->addLayout(chartRow, 1);

    connect(m_paramBox, &QComboBox::currentIndexChanged, this, &GraphPanel::onParamChanged);
    onParamChanged();
    applyTheme();
}

void GraphPanel::onParamChanged()
{
    const int index = m_paramBox->currentIndex();
    const bool remote = index >= m_remoteParamOffset;
    QString yTitle;
    if (remote) {
        yTitle = m_paramBox->currentText()
                 + (index == m_remoteParamOffset || index == m_remoteParamOffset + 2
                        ? " (us)" : " (map)");
        for (int i = 0; i < 4; ++i) {
            m_corner[i]->setVisible(i == 0);
            if (i == 0) m_corner[i]->setText("Remote");
        }
    } else {
        const Param &p = parameters().at(index);
        yTitle = p.unit.isEmpty() ? p.name : QString("%1 (%2)").arg(p.name, p.unit);
        const QStringList labels = AppState::cornerLabels();
        for (int i = 0; i < 4; ++i) {
            m_corner[i]->setVisible(true);
            m_corner[i]->setText(labels.at(i));
        }
    }
    m_axisY->setTitleText(yTitle);
    clearData();
}

void GraphPanel::clearData()
{
    for (int i = 0; i < 4; ++i) {
        m_points[i].clear();
        m_series[i]->clear();
    }
    m_lastTickMs = 0;
}

void GraphPanel::tick(qint64 nowMs, bool paused)
{
    if (paused) {
        m_lastTickMs = nowMs;
        return;
    }

    const qint64 elapsedMs = m_lastTickMs > 0
        ? std::clamp(nowMs - m_lastTickMs, qint64(0), qint64(500))
        : 0;
    m_lastTickMs = nowMs;
    const double shiftSec = elapsedMs * kScrollSpeed / 1000.0;

    auto &app             = AppState::instance();
    const int paramIndex = m_paramBox->currentIndex();
    const bool remote = paramIndex >= m_remoteParamOffset;
    const Param *p = remote ? nullptr : &parameters().at(paramIndex);
    const qint64 startMs  = nowMs - kWindowMs;

    double minV = std::numeric_limits<double>::infinity();
    double maxV = -std::numeric_limits<double>::infinity();

    for (int i = 0; i < 4; ++i) {
        if (remote && i > 0) {
            m_points[i].clear();
            m_series[i]->clear();
            m_series[i]->setVisible(false);
            continue;
        }
        const auto t      = app.monitor.latest(app.cornerMac(i));
        const bool online = t && (nowMs - t->timeMs) < kOfflineMs;

        // Titik baru (X = detik relatif terhadap sekarang)
        if (remote) {
            const auto vcu = app.monitor.latestVcu(app.vcuMac());
            const bool vcuOnline = vcu && (nowMs - vcu->timeMs) < kOfflineMs;
            if (vcuOnline) {
                double value = 0;
                switch (paramIndex - m_remoteParamOffset) {
                case 0: value = vcu->steerRemoteRaw; break;
                case 1: value = vcu->steerRemoteMap; break;
                case 2: value = vcu->throttleRemoteRaw; break;
                case 3: value = vcu->throttleRemoteMap; break;
                }
                m_points[i].append(QPointF(0.0, value));
            }
        } else if (online && p->valid(*t)) {
            m_points[i].append(QPointF(0.0, p->value(*t)));
        }

        // Geser berdasarkan waktu nyata agar timer yang terlambat tidak memperlambat grafik.
        int drop = 0;
        for (auto &pt : m_points[i]) {
            pt.rx() -= shiftSec;
            if (pt.x() < -60.0) ++drop;
        }
        if (drop > 0) m_points[i].remove(0, drop);

        m_series[i]->replace(m_points[i]);
        m_series[i]->setName(remote ? "Remote" : AppState::cornerLabels().at(i));
        m_series[i]->setVisible(m_corner[i]->isChecked());

        if (m_corner[i]->isChecked()) {
            for (const QPointF &pt : std::as_const(m_points[i])) {
                minV = std::min(minV, pt.y());
                maxV = std::max(maxV, pt.y());
            }
        }
    }

    if (m_autoChk->isChecked()) {
        m_axisX->setRange(-60, 0);

        if (minV <= maxV) {
            const double span = maxV - minV;
            const double pad  = span < 1e-9 ? std::max(1.0, std::abs(maxV) * 0.1)
                                            : span * 0.1;
            m_axisY->setRange(minV - pad, maxV + pad);
            m_axisY->applyNiceNumbers();
        } else {
            // Tidak ada data – tampilkan sumbu default
            m_axisY->setRange(-1, 1);
        }
    }
}

void GraphPanel::applyTheme()
{
    m_chart->setTheme(QChart::ChartThemeLight);   // selalu light, abaikan tema OS

    // Re-apply warna seri (theme override warna)
    static const QColor kColors[4] = {
        QColor("#4472C4"),
        QColor("#ED7D31"),
        QColor("#70AD47"),
        QColor("#FF0000"),
    };
    for (int i = 0; i < 4; ++i) {
        QPen pen(kColors[i]);
        pen.setWidth(2);
        m_series[i]->setPen(pen);
    }
}

void GraphPanel::changeEvent(QEvent *e)
{
    if (e->type() == QEvent::PaletteChange || e->type() == QEvent::ThemeChange)
        applyTheme();
    QWidget::changeEvent(e);
}


// ============================================================
//  GraphPage
// ============================================================
GraphPage::GraphPage(QWidget *parent) : QWidget(parent)
{
    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(12, 10, 12, 10);
    root->setSpacing(6);

    // Header: Judul + Kontrol Global (Pause & Clear All)
    auto *headerLayout = new QHBoxLayout;
    headerLayout->addStretch(1);

    m_pauseChk = new QCheckBox("Pause");
    headerLayout->addWidget(m_pauseChk);

    auto *clearBtn = new QPushButton("Clear All");
    clearBtn->setFixedWidth(100);
    connect(clearBtn, &QPushButton::clicked, this, [this] {
        for (int i = 0; i < 2; ++i)
            m_panel[i]->clearData();
    });
    headerLayout->addWidget(clearBtn);

    root->addLayout(headerLayout);

    // Dua panel grafik dalam satu baris (kiri dan kanan).
    for (int i = 0; i < 2; ++i)
        m_panel[i] = new GraphPanel(this);

    // Default parameter per panel
    const int defaultIdx[2] = {0, 1};
    for (int i = 1; i < 2; ++i) {
        auto *cb = m_panel[i]->findChild<QComboBox*>();
        if (cb && cb->count() > defaultIdx[i])
            cb->setCurrentIndex(defaultIdx[i]);
    }

    // Grid satu baris dengan dua kolom.
    auto *grid = new QWidget;
    auto *gridLayout = new QGridLayout(grid);
    gridLayout->setContentsMargins(0, 0, 0, 0);
    gridLayout->setSpacing(6);
    gridLayout->addWidget(m_panel[0], 0, 0);   // kiri
    gridLayout->addWidget(m_panel[1], 0, 1);   // kanan
    gridLayout->setColumnStretch(0, 1);
    gridLayout->setColumnStretch(1, 1);
    root->addWidget(grid, 1);

    // Timer bersama
    m_timer = new QTimer(this);
    connect(m_timer, &QTimer::timeout, this, &GraphPage::tick);
    m_timer->start(50);
}

void GraphPage::tick()
{
    const qint64 nowMs = QDateTime::currentMSecsSinceEpoch();
    const bool paused  = m_pauseChk && m_pauseChk->isChecked();
    for (int i = 0; i < 2; ++i)
        m_panel[i]->tick(nowMs, paused);
}
