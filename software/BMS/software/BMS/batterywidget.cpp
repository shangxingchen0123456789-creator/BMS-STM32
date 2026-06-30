#include "batterywidget.h"
#include <QPainter>
#include <QPainterPath>

BatteryWidget::BatteryWidget(QWidget *parent)
    : QWidget(parent)
{
    setMinimumSize(80, 30);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
}

void BatteryWidget::setLevel(int percent)
{
    m_level = qBound(0, percent, 100);
    update();
}

void BatteryWidget::paintEvent(QPaintEvent * /*event*/)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    const int w = width();
    const int h = height();
    const int margin = 4;

    // 电池正极凸起尺寸
    const int tipW = 6;
    const int tipH = h / 3;

    // 电池主体区域
    QRectF body(margin, margin, w - 2 * margin - tipW, h - 2 * margin);

    // ---------- 绘制电池外壳 ----------
    p.setPen(QPen(QColor("#5a5c6e"), 2));
    p.setBrush(Qt::NoBrush);

    QPainterPath bodyPath;
    bodyPath.addRoundedRect(body, 4, 4);
    p.drawPath(bodyPath);

    // 正极凸起
    QRectF tip(body.right(), (h - tipH) / 2.0, tipW, tipH);
    p.setPen(QPen(QColor("#5a5c6e"), 2));
    p.setBrush(QColor("#5a5c6e"));
    p.drawRoundedRect(tip, 2, 2);

    // ---------- 填充电量 ----------
    if (m_level > 0) {
        QColor fillColor;
        if (m_level > 50) {
            fillColor = QColor("#66bb6a");       // 绿色
        } else if (m_level > 20) {
            fillColor = QColor("#ffa726");       // 橙色
        } else {
            fillColor = QColor("#ef5350");       // 红色
        }

        double innerMargin = 3;
        double fillW = (body.width() - 2 * innerMargin) * m_level / 100.0;
        QRectF fillRect(body.left() + innerMargin,
                        body.top() + innerMargin,
                        fillW,
                        body.height() - 2 * innerMargin);

        // 渐变填充
        QLinearGradient grad(fillRect.topLeft(), fillRect.bottomLeft());
        grad.setColorAt(0, fillColor.lighter(130));
        grad.setColorAt(1, fillColor);
        p.setPen(Qt::NoPen);
        p.setBrush(grad);
        p.drawRoundedRect(fillRect, 2, 2);
    }

    // ---------- 百分比文字 ----------
    p.setPen(QColor("#ffffff"));
    QFont f = font();
    f.setPointSize(qMax(8, h / 4));
    f.setBold(true);
    p.setFont(f);
    p.drawText(body, Qt::AlignCenter, QString("%1%").arg(m_level));
}
