#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "qcustomplot.h"
#include "batterywidget.h"
#include "ledindicator.h"
#include <QSerialPortInfo>
#include <QDateTime>
#include <QLineEdit>
#include <QStatusBar>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSplitter>
#include <QToolTip>
#include <QLabel>
#include <QGroupBox>
#include <QFormLayout>
#include <QPushButton>
#include <QSpinBox>
#include <QFrame>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , host(new BmsHost(this))
{
    ui->setupUi(this);

    // 波特率默认选 115200
    int idx = ui->comboBaud->findText("115200");
    if (idx >= 0) ui->comboBaud->setCurrentIndex(idx);

    refreshPorts();
    setupBattery();
    setupLed();
    setupCharts();
    setupBarChart();
    setupTooltip();
    setupSettings();
    setupDeviceInfo();

    connect(host, &BmsHost::ackReceived, this, &MainWindow::onAck);
    connect(host, &BmsHost::telemetryReceived, this, &MainWindow::onTelemetry);
    connect(host, &BmsHost::logLine, this, &MainWindow::onLog);

    // 初始界面
    ui->btnStop->setEnabled(false);

    plotTimer.start();
}

MainWindow::~MainWindow()
{
    delete ui;
}

// ============================================================
// SOC 电池图标
// ============================================================
void MainWindow::setupBattery()
{
    QProgressBar *bar = ui->progressSoc;
    if (!bar) return;

    batteryIcon = new BatteryWidget(bar->parentWidget());
    batteryIcon->setLevel(0);

    QLayout *parentLayout = bar->parentWidget()->layout();
    if (parentLayout) {
        parentLayout->replaceWidget(bar, batteryIcon);
    }
    bar->hide();
}

// ============================================================
// 连接状态 LED 指示灯
// ============================================================
void MainWindow::setupLed()
{
    // 用 objectName 直接找到连接按钮所在的 QHBoxLayout
    QHBoxLayout *hlay = findChild<QHBoxLayout*>("horizontalLayout_17");
    if (!hlay) return;

    ledIndicator = new LedIndicator(this);
    ledIndicator->setStatus(LedIndicator::Disconnected);
    ledIndicator->setToolTip(QString::fromUtf8("未连接"));

    // 在按钮前面插入 LED
    hlay->insertWidget(0, ledIndicator);
}

// ============================================================
// 设备信息面板（填充 BMS连接区域空白）
// ============================================================
void MainWindow::setupDeviceInfo()
{
    QGroupBox *connBox = findChild<QGroupBox*>("groupBox");
    if (!connBox) return;

    QVBoxLayout *boxLayout = qobject_cast<QVBoxLayout*>(connBox->layout());
    if (!boxLayout) return;

    // —— 刷新端口按钮 ——
    auto *btnRefresh = new QPushButton(QString::fromUtf8("↻ 刷新端口"), connBox);
    btnRefresh->setMinimumHeight(28);
    btnRefresh->setStyleSheet(
        "QPushButton { background-color: #37474f; color: #b0bec5; "
        "border: 1px solid #546e7a; border-radius: 4px; font-size: 11px; }"
        "QPushButton:hover { background-color: #455a64; color: #eceff1; }"
    );
    connect(btnRefresh, &QPushButton::clicked, this, &MainWindow::refreshPorts);
    boxLayout->addWidget(btnRefresh);

    // —— 分隔线 ——
    auto *sep = new QFrame(connBox);
    sep->setFrameShape(QFrame::HLine);
    sep->setStyleSheet("QFrame { color: #3d3f50; }");
    boxLayout->addWidget(sep);

    // —— 设备信息区域 ——
    auto *infoGroup = new QGroupBox(QString::fromUtf8("设备信息"), connBox);
    infoGroup->setStyleSheet(
        "QGroupBox { font-size: 11px; font-weight: bold; color: #90a4ae; "
        "border: 1px solid #37474f; border-radius: 4px; margin-top: 8px; padding-top: 14px; }"
        "QGroupBox::title { subcontrol-origin: margin; left: 8px; padding: 0 4px; }"
    );
    auto *infoForm = new QFormLayout(infoGroup);
    infoForm->setContentsMargins(8, 4, 8, 4);
    infoForm->setVerticalSpacing(6);

    auto makeLabel = [&](const QString &init) -> QLabel* {
        auto *lbl = new QLabel(init, infoGroup);
        lbl->setStyleSheet("QLabel { color: #cfd8dc; font-size: 11px; }");
        return lbl;
    };
    auto makeTitle = [&](const QString &title) -> QLabel* {
        auto *lbl = new QLabel(title, infoGroup);
        lbl->setStyleSheet("QLabel { color: #78909c; font-size: 11px; }");
        return lbl;
    };

    lblConnStatus  = makeLabel(QString::fromUtf8("⚫ 未连接"));
    lblConnTime    = makeLabel("--");
    lblPacketCount = makeLabel("0");
    lblFwState     = makeLabel("--");
    lblFwAlerts    = makeLabel("--");
    lblFwSocSoh    = makeLabel("--");

    infoForm->addRow(makeTitle(QString::fromUtf8("连接状态:")), lblConnStatus);
    infoForm->addRow(makeTitle(QString::fromUtf8("连接时长:")), lblConnTime);
    infoForm->addRow(makeTitle(QString::fromUtf8("收包计数:")), lblPacketCount);

    // 运行状态分隔
    auto *sep2 = new QFrame(infoGroup);
    sep2->setFrameShape(QFrame::HLine);
    sep2->setStyleSheet("QFrame { color: #37474f; }");
    infoForm->addRow(sep2);

    infoForm->addRow(makeTitle(QString::fromUtf8("运行状态:")), lblFwState);
    infoForm->addRow(makeTitle(QString::fromUtf8("告警标志:")), lblFwAlerts);
    infoForm->addRow(makeTitle(QString::fromUtf8("SOC/SOH:")), lblFwSocSoh);

    boxLayout->addWidget(infoGroup);

    // 弹性空白推到底部
    boxLayout->addStretch();
}

// ============================================================
// 图表初始化: 电芯电压曲线 + 总览曲线
// ============================================================
void MainWindow::setupCharts()
{
    QWidget *container = findChild<QWidget*>("chartWidget");
    if (!container) return;

    auto *layout = new QVBoxLayout(container);
    layout->setContentsMargins(4, 4, 4, 4);
    layout->setSpacing(8);

    auto *splitter = new QSplitter(Qt::Vertical, container);
    layout->addWidget(splitter);

    // --------- 图表1: 15路电芯电压 ---------
    voltagePlot = new QCustomPlot(splitter);
    voltagePlot->setMinimumHeight(150);
    voltagePlot->setBackground(QColor("#2b2d3a"));
    voltagePlot->xAxis->setLabel(QString::fromUtf8("时间 (s)"));
    voltagePlot->yAxis->setLabel(QString::fromUtf8("电芯电压 (mV)"));
    voltagePlot->yAxis->setRange(2500, 4300);

    auto styleAxis = [](QCPAxis *axis) {
        axis->setBasePen(QPen(QColor("#5a5c6e")));
        axis->setTickPen(QPen(QColor("#5a5c6e")));
        axis->setSubTickPen(QPen(QColor("#3d3f50")));
        axis->setTickLabelColor(QColor("#a0a0b0"));
        axis->setLabelColor(QColor("#b0b8c8"));
        axis->grid()->setPen(QPen(QColor("#3d3f50"), 1, Qt::DotLine));
    };
    styleAxis(voltagePlot->xAxis);
    styleAxis(voltagePlot->yAxis);
    voltagePlot->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom);

    for (int i = 0; i < CELL_COUNT; ++i) {
        voltagePlot->addGraph();
        QColor color = QColor::fromHsv(i * (360 / CELL_COUNT), 200, 220);
        voltagePlot->graph(i)->setPen(QPen(color, 1.5));
        voltagePlot->graph(i)->setName(QString("Cell %1").arg(i + 1));
    }

    voltagePlot->legend->setVisible(true);
    voltagePlot->legend->setBrush(QColor(43, 45, 58, 200));
    voltagePlot->legend->setTextColor(QColor("#e0e0e0"));
    voltagePlot->legend->setBorderPen(QPen(QColor("#3d3f50")));
    QFont legendFont = font();
    legendFont.setPointSize(8);
    voltagePlot->legend->setFont(legendFont);
    voltagePlot->axisRect()->insetLayout()->setInsetAlignment(0, Qt::AlignTop | Qt::AlignRight);

    splitter->addWidget(voltagePlot);

    // --------- 图表2: 总电压 / 电流 / 温度 ---------
    overviewPlot = new QCustomPlot(splitter);
    overviewPlot->setMinimumHeight(120);
    overviewPlot->setBackground(QColor("#2b2d3a"));

    overviewPlot->yAxis->setLabel(QString::fromUtf8("电压(mV) / 电流(mA)"));
    styleAxis(overviewPlot->xAxis);
    styleAxis(overviewPlot->yAxis);

    overviewPlot->yAxis2->setVisible(true);
    overviewPlot->yAxis2->setLabel(QString::fromUtf8("温度 (°C)"));
    styleAxis(overviewPlot->yAxis2);
    overviewPlot->yAxis2->setRange(-10, 60);

    overviewPlot->xAxis->setLabel(QString::fromUtf8("时间 (s)"));
    overviewPlot->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom);

    overviewPlot->addGraph(overviewPlot->xAxis, overviewPlot->yAxis);
    overviewPlot->graph(0)->setPen(QPen(QColor("#4fc3f7"), 2));
    overviewPlot->graph(0)->setName(QString::fromUtf8("总电压"));

    overviewPlot->addGraph(overviewPlot->xAxis, overviewPlot->yAxis);
    overviewPlot->graph(1)->setPen(QPen(QColor("#ffa726"), 2));
    overviewPlot->graph(1)->setName(QString::fromUtf8("电流"));

    overviewPlot->addGraph(overviewPlot->xAxis, overviewPlot->yAxis2);
    overviewPlot->graph(2)->setPen(QPen(QColor("#ef5350"), 2, Qt::DashLine));
    overviewPlot->graph(2)->setName(QString::fromUtf8("温度"));

    overviewPlot->legend->setVisible(true);
    overviewPlot->legend->setBrush(QColor(43, 45, 58, 200));
    overviewPlot->legend->setTextColor(QColor("#e0e0e0"));
    overviewPlot->legend->setBorderPen(QPen(QColor("#3d3f50")));
    overviewPlot->legend->setFont(legendFont);
    overviewPlot->axisRect()->insetLayout()->setInsetAlignment(0, Qt::AlignTop | Qt::AlignRight);

    splitter->addWidget(overviewPlot);
    splitter->setStretchFactor(0, 3);
    splitter->setStretchFactor(1, 1);
}

// ============================================================
// 电芯柱状图 (第三个 Tab)
// ============================================================
void MainWindow::setupBarChart()
{
    QWidget *barContainer = findChild<QWidget*>("barChartWidget");
    if (!barContainer) return;

    auto *layout = new QVBoxLayout(barContainer);
    layout->setContentsMargins(8, 8, 8, 8);

    barPlot = new QCustomPlot(barContainer);
    layout->addWidget(barPlot);

    barPlot->setBackground(QColor("#2b2d3a"));

    auto styleAxis = [](QCPAxis *axis) {
        axis->setBasePen(QPen(QColor("#5a5c6e")));
        axis->setTickPen(QPen(QColor("#5a5c6e")));
        axis->setSubTickPen(QPen(QColor("#3d3f50")));
        axis->setTickLabelColor(QColor("#a0a0b0"));
        axis->setLabelColor(QColor("#b0b8c8"));
        axis->grid()->setPen(QPen(QColor("#3d3f50"), 1, Qt::DotLine));
    };

    // X 轴: 电芯编号
    QVector<double> ticks;
    QVector<QString> labels;
    for (int i = 1; i <= CELL_COUNT; ++i) {
        ticks << i;
        labels << QString::number(i);
    }
    QSharedPointer<QCPAxisTickerText> textTicker(new QCPAxisTickerText);
    textTicker->addTicks(ticks, labels);
    barPlot->xAxis->setTicker(textTicker);
    barPlot->xAxis->setLabel(QString::fromUtf8("电芯编号"));
    barPlot->xAxis->setRange(0, CELL_COUNT + 1);
    styleAxis(barPlot->xAxis);

    // Y 轴: 电压
    barPlot->yAxis->setLabel(QString::fromUtf8("电压 (mV)"));
    barPlot->yAxis->setRange(2500, 4300);
    styleAxis(barPlot->yAxis);

    // 创建柱状图
    cellBars = new QCPBars(barPlot->xAxis, barPlot->yAxis);
    cellBars->setWidth(0.6);
    cellBars->setName(QString::fromUtf8("电芯电压"));

    // 渐变笔刷
    cellBars->setPen(QPen(QColor("#4fc3f7").darker(120)));
    cellBars->setBrush(QColor("#4fc3f7"));

    // 初始数据 (全0)
    QVector<double> keys, values;
    for (int i = 1; i <= CELL_COUNT; ++i) {
        keys << i;
        values << 0;
    }
    cellBars->setData(keys, values);

    barPlot->setInteractions(QCP::iRangeZoom | QCP::iRangeDrag);
    barPlot->replot();
}

// ============================================================
// 图表悬停提示
// ============================================================
void MainWindow::setupTooltip()
{
    if (voltagePlot) {
        voltagePlot->setMouseTracking(true);
        connect(voltagePlot, &QCustomPlot::mouseMove, this, &MainWindow::onPlotMouseMove);
    }
    if (overviewPlot) {
        overviewPlot->setMouseTracking(true);
        connect(overviewPlot, &QCustomPlot::mouseMove, this, &MainWindow::onPlotMouseMove);
    }
    if (barPlot) {
        barPlot->setMouseTracking(true);
        connect(barPlot, &QCustomPlot::mouseMove, this, &MainWindow::onPlotMouseMove);
    }
}

void MainWindow::onPlotMouseMove(QMouseEvent *event)
{
    QCustomPlot *plot = qobject_cast<QCustomPlot*>(sender());
    if (!plot) return;

    // 查找离鼠标最近的数据点
    double minDist = 1e9;
    QString tipText;

    for (int i = 0; i < plot->graphCount(); ++i) {
        QCPGraph *graph = plot->graph(i);
        if (!graph || graph->data()->isEmpty()) continue;

        double coordX = graph->keyAxis()->pixelToCoord(event->pos().x());
        QCPGraphDataContainer::const_iterator it = graph->data()->findBegin(coordX, false);
        if (it == graph->data()->constEnd()) continue;

        double px = graph->keyAxis()->coordToPixel(it->key);
        double py = graph->valueAxis()->coordToPixel(it->value);
        double dist = qSqrt(qPow(event->pos().x() - px, 2) + qPow(event->pos().y() - py, 2));

        if (dist < 30 && dist < minDist) {
            minDist = dist;
            tipText = QString("%1\n%2: %3")
                          .arg(graph->name())
                          .arg(QString::fromUtf8("值"))
                          .arg(QString::number(it->value, 'f', 1));
            if (plot == voltagePlot || plot == overviewPlot) {
                tipText += QString("\n%1: %2s")
                               .arg(QString::fromUtf8("时间"))
                               .arg(QString::number(it->key, 'f', 1));
            }
        }
    }

    // 也检查 QCPBars
    if (plot == barPlot && cellBars) {
        double coordX = plot->xAxis->pixelToCoord(event->pos().x());
        int cellIdx = qRound(coordX);
        if (cellIdx >= 1 && cellIdx <= CELL_COUNT) {
            QCPBarsDataContainer::const_iterator it = cellBars->data()->findBegin(cellIdx, false);
            if (it != cellBars->data()->constEnd() && qAbs(it->key - cellIdx) < 0.5) {
                double px = plot->xAxis->coordToPixel(it->key);
                double py = plot->yAxis->coordToPixel(it->value);
                double dist = qSqrt(qPow(event->pos().x() - px, 2) + qPow(event->pos().y() - py, 2));
                if (dist < 50) {
                    tipText = QString("Cell %1\n%2: %3 mV")
                                  .arg(cellIdx)
                                  .arg(QString::fromUtf8("电压"))
                                  .arg(QString::number(it->value, 'f', 0));
                }
            }
        }
    }

    if (!tipText.isEmpty()) {
        QToolTip::showText(event->globalPos(), tipText, plot);
    } else {
        QToolTip::hideText();
    }
}

// ============================================================
// 告警高亮
// ============================================================
void MainWindow::updateCellAlert(int index, uint16_t mv)
{
    QLineEdit *le = findChild<QLineEdit*>(QString("cellEdit%1").arg(index + 1));
    if (!le) return;

    if (mv > cellOvThresh) {
        le->setStyleSheet("QLineEdit { background-color: #5c1a1a; color: #ff6b6b; border-color: #ef5350; }");
    } else if (mv < cellUvThresh) {
        le->setStyleSheet("QLineEdit { background-color: #5c3a1a; color: #ffab40; border-color: #ffa726; }");
    } else {
        le->setStyleSheet("");
    }
}

void MainWindow::updateMosIndicator(QWidget *w, bool on)
{
    if (!w) return;
    if (on) {
        w->setStyleSheet("QLineEdit { background-color: #1b5e20; color: #66bb6a; border-color: #43a047; font-weight: bold; }");
    } else {
        w->setStyleSheet("QLineEdit { background-color: #4a1a1a; color: #ef5350; border-color: #c62828; font-weight: bold; }");
    }
}

// ============================================================
// 原有逻辑
// ============================================================
void MainWindow::refreshPorts()
{
    ui->comboPort->clear();
    for (const auto &info : QSerialPortInfo::availablePorts()) {
        ui->comboPort->addItem(info.portName());
    }
}

void MainWindow::on_btnConnect_clicked()
{
    if (!host->isOpen()) {
        // 尝试连接
        if (ledIndicator) {
            ledIndicator->setStatus(LedIndicator::Connecting);
            ledIndicator->setToolTip(QString::fromUtf8("连接中..."));
        }

        QString port = ui->comboPort->currentText();
        bool ok;
        int baud = ui->comboBaud->currentText().toInt(&ok);
        if (!ok) baud = 115200;

        if (host->open(port, baud)) {
            ui->btnConnect->setText(QString::fromUtf8("断开连接"));
            if (ledIndicator) {
                ledIndicator->setStatus(LedIndicator::Connected);
                ledIndicator->setToolTip(QString::fromUtf8("已连接: %1").arg(port));
            }
            if (lblConnStatus) {
                lblConnStatus->setText(QString::fromUtf8("🟢 已连接 (%1)").arg(port));
                lblConnStatus->setStyleSheet("QLabel { color: #66bb6a; font-size: 11px; font-weight: bold; }");
            }
            connTimer.start();
            packetCount = 0;
            if (lblPacketCount) lblPacketCount->setText("0");
            if (lblConnTime) lblConnTime->setText("0s");
            host->ping();
        } else {
            if (ledIndicator) {
                ledIndicator->setStatus(LedIndicator::Disconnected);
                ledIndicator->setToolTip(QString::fromUtf8("连接失败"));
            }
        }
    } else {
        host->close();
        ui->btnConnect->setText(QString::fromUtf8("连接BMS"));
        ui->btnStop->setEnabled(false);
        ui->btnStart->setEnabled(true);
        if (ledIndicator) {
            ledIndicator->setStatus(LedIndicator::Disconnected);
            ledIndicator->setToolTip(QString::fromUtf8("未连接"));
        }
        if (lblConnStatus) {
            lblConnStatus->setText(QString::fromUtf8("⚫ 未连接"));
            lblConnStatus->setStyleSheet("QLabel { color: #cfd8dc; font-size: 11px; }");
        }
        if (lblConnTime) lblConnTime->setText("--");
        if (lblFwState) lblFwState->setText("--");
        if (lblFwAlerts) lblFwAlerts->setText("--");
        if (lblFwSocSoh) lblFwSocSoh->setText("--");
    }
}

void MainWindow::on_btnStart_clicked()
{
    if (!host->isOpen()) return;
    host->streamStart();
    ui->btnStart->setEnabled(false);
    ui->btnStop->setEnabled(true);
}

void MainWindow::on_btnStop_clicked()
{
    if (!host->isOpen()) return;
    host->streamStop();
    ui->btnStop->setEnabled(false);
    ui->btnStart->setEnabled(true);
}

void MainWindow::on_btnChgOn_clicked()  { if (host->isOpen()) host->chgOn();  }
void MainWindow::on_btnChgOff_clicked() { if (host->isOpen()) host->chgOff(); }
void MainWindow::on_btnDsgOn_clicked()  { if (host->isOpen()) host->dsgOn();  }
void MainWindow::on_btnDsgOff_clicked() { if (host->isOpen()) host->dsgOff(); }

void MainWindow::onAck(quint8 seq, quint8 err)
{
    QString s = (err == BmsProto::COMM_OK) ? "OK" : QString("ERR=0x%1").arg(err,2,16,QChar('0'));
    onLog(QString("ACK seq=%1 %2").arg(seq).arg(s));
}

QString MainWindow::fmtTemp(qint16 x10) const
{
    if (x10 == qint16(0x8000)) return "--";
    return QString::number(double(x10)/10.0, 'f', 1);
}

void MainWindow::onTelemetry(quint8 /*seq*/, const BmsProto::Telemetry& t)
{
    // ---------- 设备信息面板 ----------
    updateDeviceInfo(t);
    if (batteryIcon) batteryIcon->setLevel(t.soc);
    ui->progressSoc->setValue(t.soc);

    // ---------- MOS 状态 ----------
    ui->labelChgState->setText(t.chg_enable ? "ON" : "OFF");
    ui->labelDsgState->setText(t.dsg_enable ? "ON" : "OFF");
    updateMosIndicator(ui->labelChgState, t.chg_enable);
    updateMosIndicator(ui->labelDsgState, t.dsg_enable);

    // ---------- 电芯电压 + 告警 + 柱状图 ----------
    QVector<double> barKeys, barValues;
    for (int i = 0; i < 15; ++i) {
        QLineEdit* le = findChild<QLineEdit*>(QString("cellEdit%1").arg(i + 1));
        if (!le) continue;

        if (i < t.cell_mv.size() && !BmsProto::isInvalidCell(t.cell_mv[i])) {
            le->setText(QString::number(t.cell_mv[i]));
            updateCellAlert(i, t.cell_mv[i]);
            barKeys << (i + 1);
            barValues << t.cell_mv[i];
        } else {
            le->setText("--");
            le->setStyleSheet("");
            barKeys << (i + 1);
            barValues << 0;
        }
    }

    // 更新柱状图数据 + 颜色
    if (cellBars && barPlot) {
        cellBars->setData(barKeys, barValues);

        // 找出最大最小值用于颜色编码
        double minV = 9999, maxV = 0;
        for (double v : barValues) {
            if (v > 0) {
                if (v < minV) minV = v;
                if (v > maxV) maxV = v;
            }
        }
        double spread = maxV - minV;

        // 根据一致性设置颜色
        if (spread < 50) {
            // 高一致性: 绿色
            cellBars->setBrush(QColor("#66bb6a"));
            cellBars->setPen(QPen(QColor("#388e3c")));
        } else if (spread < 200) {
            // 中等: 蓝色
            cellBars->setBrush(QColor("#4fc3f7"));
            cellBars->setPen(QPen(QColor("#0288d1")));
        } else {
            // 不均衡: 橙色警告
            cellBars->setBrush(QColor("#ffa726"));
            cellBars->setPen(QPen(QColor("#e65100")));
        }

        barPlot->yAxis->rescale(true);
        double yLow = barPlot->yAxis->range().lower - 200;
        double yHigh = barPlot->yAxis->range().upper + 200;
        if (yLow < 0) yLow = 0;
        barPlot->yAxis->setRange(yLow, yHigh);
        barPlot->replot(QCustomPlot::rpQueuedReplot);
    }

    // ---------- 总电压/电流/温度 ----------
    ui->linePackV->setText(BmsProto::isInvalid(t.pack_voltage) ? "--" : QString::number(t.pack_voltage));
    ui->lineCurrent->setText(BmsProto::isInvalid(t.current) ? "--" : QString::number(t.current));
    ui->lineTemp->setText(QString("%1 / %2 / %3")
                              .arg(fmtTemp(t.t0_x10))
                              .arg(fmtTemp(t.t1_x10))
                              .arg(fmtTemp(t.t2_x10)));

    auto stateToText = [](quint8 state) -> QString {
        switch (state) {
        case 1: return "CHARGING";
        case 2: return "DISCHARGING";
        case 3: return "FAULT";
        default: return "IDLE";
        }
    };

    const double capAh = t.learned_capacity_10mAh / 100.0;
    const double remAh = t.remaining_capacity_10mAh / 100.0;
    QString status = QString("SOH=%1% Cycle=%2 Cap=%3Ah Rem=%4Ah State=%5 Alerts=0x%6")
                         .arg(t.soh)
                         .arg(t.cycle_count)
                         .arg(QString::number(capAh, 'f', 2))
                         .arg(QString::number(remAh, 'f', 2))
                         .arg(stateToText(t.state))
                         .arg(t.alerts, 4, 16, QChar('0'));

    if (ui->statusbar) {
        ui->statusbar->showMessage(status);
    } else {
        statusBar()->showMessage(status);
    }

    // ---------- 更新电芯电压曲线 ----------
    if (voltagePlot) {
        double elapsed = plotTimer.elapsed() / 1000.0;
        for (int i = 0; i < CELL_COUNT && i < t.cell_mv.size(); ++i) {
            if (!BmsProto::isInvalidCell(t.cell_mv[i])) {
                voltagePlot->graph(i)->addData(elapsed, t.cell_mv[i]);
            }
        }
        double lower = (elapsed > plotWindowSec) ? (elapsed - plotWindowSec) : 0;
        voltagePlot->xAxis->setRange(lower, elapsed + 2);
        voltagePlot->yAxis->rescale(true);
        double yLow = voltagePlot->yAxis->range().lower - 100;
        double yHigh = voltagePlot->yAxis->range().upper + 100;
        if (yLow < 0) yLow = 0;
        voltagePlot->yAxis->setRange(yLow, yHigh);
        voltagePlot->replot(QCustomPlot::rpQueuedReplot);
    }

    // ---------- 更新总览曲线 ----------
    if (overviewPlot) {
        double elapsed = plotTimer.elapsed() / 1000.0;
        if (!BmsProto::isInvalid(t.pack_voltage))
            overviewPlot->graph(0)->addData(elapsed, t.pack_voltage);
        if (!BmsProto::isInvalid(t.current))
            overviewPlot->graph(1)->addData(elapsed, t.current);

        double temp = 0;
        if (t.t0_x10 != qint16(0x8000)) temp = t.t0_x10 / 10.0;
        else if (t.t1_x10 != qint16(0x8000)) temp = t.t1_x10 / 10.0;
        else if (t.t2_x10 != qint16(0x8000)) temp = t.t2_x10 / 10.0;
        overviewPlot->graph(2)->addData(elapsed, temp);

        double lower = (elapsed > plotWindowSec) ? (elapsed - plotWindowSec) : 0;
        overviewPlot->xAxis->setRange(lower, elapsed + 2);
        overviewPlot->yAxis->rescale(true);
        double yLow = overviewPlot->yAxis->range().lower - 500;
        double yHigh = overviewPlot->yAxis->range().upper + 500;
        overviewPlot->yAxis->setRange(yLow, yHigh);
        overviewPlot->replot(QCustomPlot::rpQueuedReplot);
    }
}

void MainWindow::onLog(const QString& line)
{
    ui->plainTextEdit->append(QString("[%1] %2")
                            .arg(QDateTime::currentDateTime().toString("HH:mm:ss"))
                            .arg(line));
}

// ============================================================
// 设备信息更新（每次收到遥测帧时调用）
// ============================================================
void MainWindow::updateDeviceInfo(const BmsProto::Telemetry& t)
{
    ++packetCount;

    // 连接时长
    if (lblConnTime && connTimer.isValid()) {
        qint64 sec = connTimer.elapsed() / 1000;
        if (sec < 60)
            lblConnTime->setText(QString("%1s").arg(sec));
        else if (sec < 3600)
            lblConnTime->setText(QString("%1m %2s").arg(sec / 60).arg(sec % 60));
        else
            lblConnTime->setText(QString("%1h %2m").arg(sec / 3600).arg((sec % 3600) / 60));
    }

    // 收包计数
    if (lblPacketCount)
        lblPacketCount->setText(QString::number(packetCount));

    // 运行状态
    if (lblFwState) {
        QString stateStr;
        QString stateColor;
        switch (t.state) {
        case 0:  stateStr = QString::fromUtf8("IDLE — 空闲");    stateColor = "#90a4ae"; break;
        case 1:  stateStr = QString::fromUtf8("CHARGING — 充电"); stateColor = "#66bb6a"; break;
        case 2:  stateStr = QString::fromUtf8("DISCHARGING — 放电"); stateColor = "#4fc3f7"; break;
        case 3:  stateStr = QString::fromUtf8("FAULT — 故障");   stateColor = "#ef5350"; break;
        default: stateStr = QString("0x%1").arg(t.state, 2, 16, QChar('0')); stateColor = "#cfd8dc"; break;
        }
        lblFwState->setText(stateStr);
        lblFwState->setStyleSheet(QString("QLabel { color: %1; font-size: 11px; font-weight: bold; }").arg(stateColor));
    }

    // 告警标志
    if (lblFwAlerts) {
        if (t.alerts == 0) {
            lblFwAlerts->setText(QString::fromUtf8("✔ 无告警"));
            lblFwAlerts->setStyleSheet("QLabel { color: #66bb6a; font-size: 11px; }");
        } else {
            lblFwAlerts->setText(QString("0x%1").arg(t.alerts, 4, 16, QChar('0')));
            lblFwAlerts->setStyleSheet("QLabel { color: #ef5350; font-size: 11px; font-weight: bold; }");
        }
    }

    // SOC / SOH
    if (lblFwSocSoh) {
        lblFwSocSoh->setText(QString("SOC %1%  |  SOH %2%").arg(t.soc).arg(t.soh));
        QString socColor = (t.soc > 50) ? "#66bb6a" : (t.soc > 20 ? "#ffa726" : "#ef5350");
        lblFwSocSoh->setStyleSheet(QString("QLabel { color: %1; font-size: 11px; font-weight: bold; }").arg(socColor));
    }
}

// ============================================================
// 模拟模式（预留）
// ============================================================
void MainWindow::onSimToggled(bool on)
{
    Q_UNUSED(on);
    // TODO: 模拟模式开关
}

void MainWindow::onSimTick()
{
    // TODO: 模拟模式定时器
}

// ============================================================
// 设置面板: 下位机阈值配置
// ============================================================
void MainWindow::setupSettings()
{
    QWidget *container = findChild<QWidget*>("settingsWidget");
    if (!container) return;

    auto *mainLayout = new QVBoxLayout(container);
    mainLayout->setContentsMargins(12, 12, 12, 12);
    mainLayout->setSpacing(16);

    // -------- 标题 --------
    auto *titleLabel = new QLabel(QString::fromUtf8("下位机保护阈值配置"), container);
    titleLabel->setStyleSheet("font-size: 16px; font-weight: bold; color: #e0e0e0;");
    mainLayout->addWidget(titleLabel);

    // -------- 电压阈值组 --------
    auto *voltGroup = new QGroupBox(QString::fromUtf8("电压阈值 (mV)"), container);
    auto *voltForm = new QFormLayout(voltGroup);

    spinCellOv = new QSpinBox(voltGroup);
    spinCellOv->setRange(3000, 4500);
    spinCellOv->setValue(4200);
    spinCellOv->setSuffix(" mV");
    voltForm->addRow(QString::fromUtf8("单体过压阈值:"), spinCellOv);

    spinCellOvRelease = new QSpinBox(voltGroup);
    spinCellOvRelease->setRange(3000, 4500);
    spinCellOvRelease->setValue(4100);
    spinCellOvRelease->setSuffix(" mV");
    voltForm->addRow(QString::fromUtf8("单体过压释放:"), spinCellOvRelease);

    spinCellUv = new QSpinBox(voltGroup);
    spinCellUv->setRange(2000, 3500);
    spinCellUv->setValue(2700);
    spinCellUv->setSuffix(" mV");
    voltForm->addRow(QString::fromUtf8("单体欠压阈值:"), spinCellUv);

    spinCellUvRelease = new QSpinBox(voltGroup);
    spinCellUvRelease->setRange(2000, 3500);
    spinCellUvRelease->setValue(2850);
    spinCellUvRelease->setSuffix(" mV");
    voltForm->addRow(QString::fromUtf8("单体欠压释放:"), spinCellUvRelease);

    mainLayout->addWidget(voltGroup);

    // -------- 电流阈值组 --------
    auto *curGroup = new QGroupBox(QString::fromUtf8("电流阈值"), container);
    auto *curForm = new QFormLayout(curGroup);

    spinOcd = new QSpinBox(curGroup);
    spinOcd->setRange(1000, 500000);
    spinOcd->setValue(100000);
    spinOcd->setSuffix(" mA");
    spinOcd->setSingleStep(1000);
    curForm->addRow(QString::fromUtf8("过流保护阈值:"), spinOcd);

    mainLayout->addWidget(curGroup);

    // -------- 温度阈值组 --------
    auto *tempGroup = new QGroupBox(QString::fromUtf8("温度阈值 (°C)"), container);
    auto *tempForm = new QFormLayout(tempGroup);

    spinChgOt = new QSpinBox(tempGroup);
    spinChgOt->setRange(0, 80);
    spinChgOt->setValue(45);
    spinChgOt->setSuffix(QString::fromUtf8(" °C"));
    tempForm->addRow(QString::fromUtf8("充电高温上限:"), spinChgOt);

    spinChgUt = new QSpinBox(tempGroup);
    spinChgUt->setRange(-40, 30);
    spinChgUt->setValue(0);
    spinChgUt->setSuffix(QString::fromUtf8(" °C"));
    tempForm->addRow(QString::fromUtf8("充电低温下限:"), spinChgUt);

    spinDsgOt = new QSpinBox(tempGroup);
    spinDsgOt->setRange(0, 80);
    spinDsgOt->setValue(60);
    spinDsgOt->setSuffix(QString::fromUtf8(" °C"));
    tempForm->addRow(QString::fromUtf8("放电高温上限:"), spinDsgOt);

    spinDsgUt = new QSpinBox(tempGroup);
    spinDsgUt->setRange(-40, 30);
    spinDsgUt->setValue(-20);
    spinDsgUt->setSuffix(QString::fromUtf8(" °C"));
    tempForm->addRow(QString::fromUtf8("放电低温下限:"), spinDsgUt);

    mainLayout->addWidget(tempGroup);

    // -------- 下发按钮 --------
    btnSendThresholds = new QPushButton(QString::fromUtf8("下发阈值到 BMS"), container);
    btnSendThresholds->setMinimumHeight(40);
    btnSendThresholds->setStyleSheet(
        "QPushButton { background-color: #1565c0; color: white; font-size: 14px; "
        "font-weight: bold; border-radius: 6px; } "
        "QPushButton:hover { background-color: #1976d2; } "
        "QPushButton:pressed { background-color: #0d47a1; }"
    );
    connect(btnSendThresholds, &QPushButton::clicked,
            this, &MainWindow::on_btnSendThresholds_clicked);
    mainLayout->addWidget(btnSendThresholds);

    // -------- 弹性空白 --------
    mainLayout->addStretch();
}

void MainWindow::on_btnSendThresholds_clicked()
{
    if (!host->isOpen()) {
        onLog(QString::fromUtf8("未连接，无法下发阈值"));
        return;
    }

    quint16 cellOvMv       = static_cast<quint16>(spinCellOv->value());
    quint16 cellOvRelease  = static_cast<quint16>(spinCellOvRelease->value());
    quint16 cellUvMv       = static_cast<quint16>(spinCellUv->value());
    quint16 cellUvRelease  = static_cast<quint16>(spinCellUvRelease->value());
    quint32 ocdMa          = static_cast<quint32>(spinOcd->value());
    qint16  chgOtDc        = static_cast<qint16>(spinChgOt->value() * 10);
    qint16  chgUtDc        = static_cast<qint16>(spinChgUt->value() * 10);
    qint16  dsgOtDc        = static_cast<qint16>(spinDsgOt->value() * 10);
    qint16  dsgUtDc        = static_cast<qint16>(spinDsgUt->value() * 10);

    host->sendThresholds(cellOvMv, cellOvRelease,
                         cellUvMv, cellUvRelease,
                         ocdMa,
                         chgOtDc, chgUtDc,
                         dsgOtDc, dsgUtDc);

    onLog(QString::fromUtf8("阈值已下发: OV=%1 UV=%2 OCD=%3mA ChgOT=%4°C DsgOT=%5°C")
              .arg(cellOvMv).arg(cellUvMv).arg(ocdMa)
              .arg(spinChgOt->value()).arg(spinDsgOt->value()));
}
