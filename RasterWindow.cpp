#include "RasterWindow.h"

#include <QGradient>

RasterWindow::RasterWindow(QWindow *parent)
    : QWindow(parent)
    , m_backingStore(new QBackingStore(this))
    , is_animated(false)
{
    setGeometry(100, 100, 1024, 780);
    //startTimer(std::chrono::milliseconds(10));
}

void RasterWindow::exposeEvent(QExposeEvent *)
{
    if (isExposed())
        renderNow();
}

bool RasterWindow::isAnimated() const
{
    return is_animated;
}

void RasterWindow::setAnimated(const bool is_animated_)
{
    is_animated = is_animated_;

    if (is_animated)
        renderLater();
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
    linearGrad.setColorAt(0, QColor(0x33, 0x08, 0x67)); // morpheus den gradient
    linearGrad.setColorAt(1, QColor(0x30, 0xcf, 0xd0));

    painter.fillRect(0, 0, width(), height(), linearGrad);
    render(painter);
    painter.end();

    m_backingStore->endPaint();
    m_backingStore->flush(rect);

    if (is_animated)
        renderLater();
}

void RasterWindow::renderLater()
{
    requestUpdate();
}

bool RasterWindow::event(QEvent *event)
{
    if (event->type() == QEvent::UpdateRequest) {
        renderNow();
        return true;
    }
    return QWindow::event(event);
}

