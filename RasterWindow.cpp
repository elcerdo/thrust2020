#include "RasterWindow.h"

#include <QGradient>

RasterWindow::RasterWindow(QWindow *parent)
    : QWindow(parent)
    , m_backingStore(new QBackingStore(this))
{
    setGeometry(100, 100, 1024, 780);
    startTimer(std::chrono::milliseconds(20));
}

void RasterWindow::exposeEvent(QExposeEvent *)
{
    if (isExposed())
        renderNow();
}

void RasterWindow::resizeEvent(QResizeEvent *resizeEvent)
{
    m_backingStore->resize(resizeEvent->size());
}

void RasterWindow::renderNow()
{
    if (!isExposed())
        return;

    QRect rect(0, 0, width(), height());
    m_backingStore->beginPaint(rect);

    QPaintDevice *device = m_backingStore->paintDevice();
    QPainter painter(device);

    QLinearGradient linearGrad(QPointF(0, 0), QPointF(0, height()));
    linearGrad.setColorAt(0, QColor(0x33, 0x08, 0x67));
    linearGrad.setColorAt(1, QColor(0x30, 0xcf, 0xd0));

    painter.fillRect(0, 0, width(), height(), linearGrad);
    render(&painter);
    painter.end();

    m_backingStore->endPaint();
    m_backingStore->flush(rect);
}

void RasterWindow::renderLater()
{
    requestUpdate();
}

void RasterWindow::timerEvent(QTimerEvent *event)
{
    renderLater();
}

bool RasterWindow::event(QEvent *event)
{
    if (event->type() == QEvent::UpdateRequest) {
        renderNow();
        return true;
    }
    return QWindow::event(event);
}

