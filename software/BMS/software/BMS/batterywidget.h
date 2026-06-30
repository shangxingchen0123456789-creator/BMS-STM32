#pragma once
#include <QWidget>

class BatteryWidget : public QWidget {
    Q_OBJECT
    Q_PROPERTY(int level READ level WRITE setLevel)
public:
    explicit BatteryWidget(QWidget *parent = nullptr);
    int level() const { return m_level; }

public slots:
    void setLevel(int percent);

protected:
    void paintEvent(QPaintEvent *event) override;
    QSize sizeHint() const override { return QSize(200, 60); }
    QSize minimumSizeHint() const override { return QSize(80, 30); }

private:
    int m_level = 0;
};
