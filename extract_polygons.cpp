#include "extract_polygons.h"

#include <QSvgRenderer>
#include <QPaintDevice>
#include <QPaintEngine>

using polygons::Color;
using polygons::Poly;
using polygons::PolyToColors;

struct SvgDumpEngine : public QPaintEngine
{
    SvgDumpEngine();
    bool begin(QPaintDevice* pdev) override;
    bool end() override;
    void updateState(const QPaintEngineState& state) override;
    void drawPixmap(const QRectF& r, const QPixmap& pm, const QRectF& sr) override;
    void drawPath(const QPainterPath& path) override;
    Type type() const override;

    bool has_pen = false;
    bool has_brush = false;
    Color current_color = { 0, 0, 0, 1 };

    static Poly closed_to_poly(const QPolygonF& poly);
    static Color to_color(const QColor& color);
    PolyToColors poly_to_pen_colors;
    PolyToColors poly_to_brush_colors;;

    QTransform transform;
};

Color SvgDumpEngine::to_color(const QColor& color)
{
    using Scalar = decltype(b2Vec2::x);
    return { static_cast<Scalar>(color.redF()), static_cast<Scalar>(color.greenF()), static_cast<Scalar>(color.blueF()), static_cast<Scalar>(color.alphaF()) };
}

Poly SvgDumpEngine::closed_to_poly(const QPolygonF& poly)
{
    using Scalar = decltype(b2Vec2::x);
    assert(poly.isClosed());
    if (poly.size() < 2)
        return {};
    Poly poly_;
    for (const QPointF& point : poly)
        poly_.emplace_back(b2Vec2 { static_cast<Scalar>(point.x()), static_cast<Scalar>(point.y()) });
    poly_.pop_back();
    assert(poly_.size() + 1 == poly.size());
    return poly_;
}

SvgDumpEngine::SvgDumpEngine() : QPaintEngine(PainterPaths | PaintOutsidePaintEvent | PrimitiveTransform)
{
}

bool SvgDumpEngine::begin(QPaintDevice* pdev)
{
    has_pen = false;
    has_brush = false;
    current_color = { 0, 0, 0, 1 };
    poly_to_pen_colors.clear();
    poly_to_brush_colors.clear();
    transform = QTransform();
    return true;
}

bool SvgDumpEngine::end()
{
    return true;
}

void SvgDumpEngine::updateState(const QPaintEngineState& state)
{
    auto flags = state.state();
    if (flags.testFlag(DirtyPen))
    {
        has_pen = state.pen() != Qt::NoPen;
        if (has_pen) current_color = to_color(state.pen().color());
        flags.setFlag(DirtyPen, false);
    }
    if (flags.testFlag(DirtyBrush))
    {
        has_brush = state.brush() != Qt::NoBrush;
        if (has_brush) current_color = to_color(state.brush().color());
        flags.setFlag(DirtyBrush, false);
    }
    if (flags.testFlag(DirtyTransform))
    {
        transform = state.transform();
        flags.setFlag(DirtyTransform, false);
    }
    flags.setFlag(DirtyHints, false);
    flags.setFlag(DirtyFont, false);
    //assert(!flags); // FIXME macos
}

void SvgDumpEngine::drawPixmap(const QRectF& r, const QPixmap& pm, const QRectF& sr)
{
}

void SvgDumpEngine::drawPath(const QPainterPath& path)
{
    const auto& poly_ = path.toFillPolygon();
    if (!poly_.isClosed())
        return;

    const Poly poly = closed_to_poly(transform.map(poly_));
    //polygons::PolyHasher hasher;
    //const auto hash = hasher(poly);

    /*
    qDebug() << transform;
    qDebug() << poly_.boundingRect();
    qDebug() << transform.map(poly_).boundingRect();
    */

    if (has_brush)
    {
        //assert(poly_to_brush_colors.find(poly) == std::cend(poly_to_brush_colors));
        poly_to_brush_colors.emplace(poly, current_color);
        return;
    }

    if (has_pen)
    {
        assert(poly_to_pen_colors.find(poly) == std::cend(poly_to_pen_colors));
        poly_to_pen_colors.emplace(poly, current_color);
        return;
    }

    //assert(false); // should have pen or brush exclusively
}

QPaintEngine::Type SvgDumpEngine::type() const
{
    return QPaintEngine::User;
}

struct SvgDumpDevice : public QPaintDevice
{
    QPaintEngine* paintEngine() const override;
    int metric(PaintDeviceMetric metric) const override;
    SvgDumpEngine engine;
};

int SvgDumpDevice::metric(PaintDeviceMetric metric) const
{
    if (metric == PdmWidth) return 1024;
    if (metric == PdmHeight) return 1024;
    if (metric == PdmDpiX) return 100;
    if (metric == PdmDpiY) return 100;
    if (metric == PdmDevicePixelRatio) return 1;
    if (metric == PdmDevicePixelRatioScaled) return 1;
    return QPaintDevice::metric(metric);
}

QPaintEngine* SvgDumpDevice::paintEngine() const
{
    return const_cast<SvgDumpEngine*>(&engine);
}

std::tuple<PolyToColors, PolyToColors> polygons::extract(const std::string& filename)
{
    QSvgRenderer renderer;
    const auto load_ok = renderer.load(QString::fromUtf8(filename.c_str()));
    assert(renderer.isValid());
    assert(load_ok);

    SvgDumpDevice device;
    {
        QPainter painter(&device);
        renderer.render(&painter, QRectF(0, 0, 1, 1));
    }

    return { device.engine.poly_to_pen_colors, device.engine.poly_to_brush_colors };
}

