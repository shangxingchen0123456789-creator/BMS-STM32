#include "ledindicator.h"
#include <QPainter>
#include <QRadialGradient>

LedIndicator::LedIndicator(QWidget *parent)
    : QWidget(parent)
{
    setFixedSize(20, 20);
}

void LedIndicator::setStatus(Status s)
{
    if (m_status != s) {
        m_status = s;
        update();
    }
}

void LedIndicator::paintEvent(QPaintEvent * /*event*/)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    QColor baseColor;
    switch (m_status) {
    case Connected:    baseColor = QColor("#66bb6a"); break;  // 绿
    case Connecting:   baseColor = QColor("#ffa726"); break;  // 黄
    case Disconnected: baseColor = QColor("#ef5350"); break;  // 红
    }

    int side = qMin(width(), height()) - 2;
    QRectF r((width() - side) / 2.0, (height() - side) / 2.0, side, side);

    // 发光效果
    QRadialGradient glow(r.center(), side);
    glow.setColorAt(0, baseColor.lighter(180));
    glow.setColorAt(0.4, baseColor);
    glow.setColorAt(1, baseColor.darker(150));

    p.setPen(QPen(baseColor.darker(200), 1));
    p.setBrush(glow);
    p.drawEllipse(r);

    // 高光
    QRectF highlight(r.left() + side * 0.25, r.top() + side * 0.15,
                     side * 0.35, side * 0.25);
    QLinearGradient hl(highlight.topLeft(), highlight.bottomLeft());
    hl.setColorAt(0, QColor(255, 255, 255, 160));
    hl.setColorAt(1, QColor(255, 255, 255, 0));
    p.setPen(Qt::NoPen);
    p.setBrush(hl);
    p.drawEllipse(highlight);
}
