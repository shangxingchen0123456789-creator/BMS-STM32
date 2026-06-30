#pragma once
#include <QMainWindow>
#include <QElapsedTimer>
#include <QTimer>
#include "bmshost.h"

class QCustomPlot;
class QCPBars;
class QSpinBox;
class QCheckBox;
class QSlider;
class QPushButton;
class BatteryWidget;
class LedIndicator;
class QLabel;

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_btnConnect_clicked();
    void on_btnStart_clicked();
    void on_btnStop_clicked();
    void on_btnChgOn_clicked();
    void on_btnChgOff_clicked();
    void on_btnDsgOn_clicked();
    void on_btnDsgOff_clicked();

    void onAck(quint8 seq, quint8 err);
    void onTelemetry(quint8 seq, const BmsProto::Telemetry& t);
    void onLog(const QString& line);

    void onPlotMouseMove(QMouseEvent *event);

    // 设置面板
    void onSimToggled(bool on);
    void onSimTick();
    void on_btnSendThresholds_clicked();

private:
    void refreshPorts();
    void setupCharts();
    void setupBarChart();
    void setupBattery();
    void setupLed();
    void setupTooltip();
    void setupSettings();
    void setupDeviceInfo();
    void updateDeviceInfo(const BmsProto::Telemetry& t);
    void updateCellAlert(int index, uint16_t mv);
    void updateMosIndicator(QWidget *w, bool on);
    QString fmtTemp(qint16 x10) const;

private:
    Ui::MainWindow *ui;
    BmsHost *host;

    BatteryWidget *batteryIcon = nullptr;
    LedIndicator *ledIndicator = nullptr;

    QCustomPlot *voltagePlot = nullptr;
    QCustomPlot *overviewPlot = nullptr;
    QCustomPlot *barPlot = nullptr;
    QCPBars *cellBars = nullptr;

    QElapsedTimer plotTimer;
    static constexpr int CELL_COUNT = 15;

    // ---- 可调参数 (设置面板) ----
    double plotWindowSec = 120.0;
    uint16_t cellOvThresh = 4250;
    uint16_t cellUvThresh = 2800;

    // 设置面板控件 (本地参数)
    QSpinBox *spinOV = nullptr;
    QSpinBox *spinUV = nullptr;
    QSpinBox *spinWindow = nullptr;
    QSpinBox *spinSimInterval = nullptr;
    QSlider *sliderSimNoise = nullptr;
    QCheckBox *chkSimMode = nullptr;

    // 设置面板控件 (下位机阈值)
    QSpinBox *spinCellOv = nullptr;
    QSpinBox *spinCellOvRelease = nullptr;
    QSpinBox *spinCellUv = nullptr;
    QSpinBox *spinCellUvRelease = nullptr;
    QSpinBox *spinOcd = nullptr;
    QSpinBox *spinChgOt = nullptr;
    QSpinBox *spinChgUt = nullptr;
    QSpinBox *spinDsgOt = nullptr;
    QSpinBox *spinDsgUt = nullptr;
    QPushButton *btnSendThresholds = nullptr;

    // 设备信息面板
    QLabel *lblConnStatus  = nullptr;
    QLabel *lblConnTime    = nullptr;
    QLabel *lblPacketCount = nullptr;
    QLabel *lblFwState     = nullptr;
    QLabel *lblFwAlerts    = nullptr;
    QLabel *lblFwSocSoh    = nullptr;
    QElapsedTimer connTimer;
    quint32 packetCount = 0;

    // 模拟模式
    QTimer *simTimer = nullptr;
    int simNoiseLevel = 50;   // 噪声幅度 mV
};
