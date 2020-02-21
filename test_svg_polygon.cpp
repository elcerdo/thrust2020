#include <QSvgRenderer>
#include <QDebug>
#include <QPaintDevice>
#include <QPaintEngine>
#include <QCoreApplication>
#include <QApplication>

struct SvgDumpEngine : public QPaintEngine
{
    SvgDumpEngine();
    bool begin(QPaintDevice* pdev) override;
    bool end() override;
    void updateState(const QPaintEngineState& state) override;
    void drawPixmap(const QRectF& r, const QPixmap& pm, const QRectF& sr) override;
    void drawPath(const QPainterPath& path) override;
    Type type() const override;
};

SvgDumpEngine::SvgDumpEngine()
    : QPaintEngine(PainterPaths | PaintOutsidePaintEvent | PrimitiveTransform)
{
}

bool SvgDumpEngine::begin(QPaintDevice* pdev)
{
    qDebug() << "begin";
    return true;
}

bool SvgDumpEngine::end()
{
    qDebug() << "end";
    return true;
}

void SvgDumpEngine::updateState(const QPaintEngineState& state)
{
    qDebug() << "udpateState" << state.state();
}

void SvgDumpEngine::drawPixmap(const QRectF& r, const QPixmap& pm, const QRectF& sr)
{
    qDebug() << "drawPixmap";
}

void SvgDumpEngine::drawPath(const QPainterPath& path)
{
    const auto& poly = path.toFillPolygon();
    qDebug() << "drawPath" << poly.size() << poly.isClosed() << path.toFillPolygon().boundingRect();
}

QPaintEngine::Type SvgDumpEngine::type() const
{
    return QPaintEngine::User;
}

struct SvgDumpDevice : public QPaintDevice
{
    QPaintEngine* paintEngine() const override;
    int metric(PaintDeviceMetric metric) const override;
    SvgDumpEngine mEngine;
};

int SvgDumpDevice::metric(PaintDeviceMetric metric) const
{
    if (metric == PdmWidth) return 1024;
    if (metric == PdmHeight) return 1024;
    if (metric == PdmDpiX) return 100;
    if (metric == PdmDpiY) return 100;
    if (metric == PdmDevicePixelRatio) return 1;
    if (metric == PdmDevicePixelRatioScaled) return 1;
    qDebug() << "metric" << metric;
    return QPaintDevice::metric(metric);
}

QPaintEngine* SvgDumpDevice::paintEngine() const
{
    return const_cast<SvgDumpEngine*>(&mEngine);
}

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);

    QSvgRenderer renderer;
    const auto load_ok = renderer.load(QString(":map.svg"));
    qDebug() << "renderer" << renderer.isValid() << load_ok;
    Q_ASSERT(renderer.isValid());

    SvgDumpDevice device;
    QPainter painter(&device);
    renderer.render(&painter);

    qDebug() << "******";

    return 0;
}

