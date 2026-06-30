#pragma once
#include <QWidget>

class LedIndicator : public QWidget {
    Q_OBJECT
    Q_PROPERTY(Status status READ status WRITE setStatus)
public:
    enum Status { Disconnected, Connecting, Connected };
    Q_ENUM(Status)

    explicit LedIndicator(QWidget *parent = nullptr);
    Status status() const { return m_status; }

public slots:
    void setStatus(Status s);

protected:
    void paintEvent(QPaintEvent *event) override;
    QSize sizeHint() const override { return QSize(20, 20); }
    QSize minimumSizeHint() const override { return QSize(12, 12); }

private:
    Status m_status = Disconnected;
};
