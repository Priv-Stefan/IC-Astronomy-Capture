#include "videowidget.h"

#include <QPainter>
#include <QPaintEvent>
#include <QLinearGradient>

VideoWidget::VideoWidget(QWidget *parent)
    : QWidget(parent)
{
    // Ensure the widget has a native window handle so IC4 can render into it.
    setAttribute(Qt::WA_NativeWindow, true);
    setAttribute(Qt::WA_OpaquePaintEvent, true);

    // Dark background so it looks like a viewfinder from the start.
    setAutoFillBackground(false);
    QPalette pal = palette();
    pal.setColor(QPalette::Window, Qt::black);
    setPalette(pal);
}

void VideoWidget::setStreamActive(bool active)
{
    if (m_streamActive != active) {
        m_streamActive = active;
        update();
    }
}

void VideoWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)

    // When IC4 is rendering, it owns the surface – we just fill with black.
    QPainter painter(this);

    // Background
    painter.fillRect(rect(), Qt::black);

    if (!m_streamActive) {
        // Draw a subtle crosshair + text placeholder
        painter.setRenderHint(QPainter::Antialiasing, true);

        // Dim grid lines
        painter.setPen(QPen(QColor(40, 40, 40), 1, Qt::SolidLine));
        const int step = 60;
        for (int x = step; x < width(); x += step)
            painter.drawLine(x, 0, x, height());
        for (int y = step; y < height(); y += step)
            painter.drawLine(0, y, width(), y);

        // Centre crosshair
        const QPoint centre = rect().center();
        painter.setPen(QPen(QColor(80, 80, 80), 1));
        painter.drawLine(centre.x() - 20, centre.y(), centre.x() + 20, centre.y());
        painter.drawLine(centre.x(), centre.y() - 20, centre.x(), centre.y() + 20);
        painter.drawEllipse(centre, 30, 30);

        // Placeholder text
        painter.setPen(QColor(120, 120, 120));
        QFont font = painter.font();
        font.setPointSize(11);
        painter.setFont(font);
        painter.drawText(rect(), Qt::AlignCenter | Qt::AlignBottom,
                         tr("No camera stream active\nSelect a camera to begin"));
    }
}
