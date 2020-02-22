#include <QSvgRenderer>
#include <QPaintDevice>
#include <QPaintEngine>
#include <QCoreApplication>
#include <QApplication>

#include <Box2D/Common/b2Math.h>

#include <boost/functional/hash.hpp>

#include <unordered_map>
#include <unordered_set>
#include <memory>
#include <iterator>
#include <iostream>
#include <iomanip>

struct SvgDumpEngine : public QPaintEngine
{
    using Color = b2Vec4;
    using Poly = std::vector<b2Vec2>;
    struct PolyHasher
    {
        size_t operator()(const Poly& poly) const;
    };
    using PolyToColors = std::unordered_map<Poly, Color, PolyHasher>;

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

SvgDumpEngine::Color SvgDumpEngine::to_color(const QColor& color)
{
    using Scalar = decltype(b2Vec2::x);
    return { static_cast<Scalar>(color.redF()), static_cast<Scalar>(color.greenF()), static_cast<Scalar>(color.blueF()), static_cast<Scalar>(color.alphaF()) };
}

SvgDumpEngine::Poly SvgDumpEngine::closed_to_poly(const QPolygonF& poly)
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

size_t SvgDumpEngine::PolyHasher::operator()(const Poly& poly) const
{
    size_t seed = 0x1fac1e5b;
    for (const auto& point : poly)
    {
        boost::hash_combine(seed, point.x);
        boost::hash_combine(seed, point.y);
    }
    return seed;
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
    assert(!flags);
}

void SvgDumpEngine::drawPixmap(const QRectF& r, const QPixmap& pm, const QRectF& sr)
{
}

void SvgDumpEngine::drawPath(const QPainterPath& path)
{
    using std::cout;
    using std::endl;
    const auto& poly_ = path.toFillPolygon();
    if (!poly_.isClosed())
        return;

    const Poly poly = closed_to_poly(poly_);
    PolyHasher hasher;
    const auto hash = hasher(poly);
    cout
        << "got poly "
        << std::setw(16) << std::setfill('0') << std::hex << hash << std::dec << " "
        << "(" << current_color.x << "|" << current_color.y << "|" << current_color.z << "|" << current_color.w << ") "
        << has_pen << " " << has_brush << " "
        << poly.size() << endl;

    if (has_brush)
    {
        assert(poly_to_brush_colors.find(poly) == std::cend(poly_to_brush_colors));
        poly_to_brush_colors.emplace(poly, current_color);
        return;
    }

    if (has_pen)
    {
        assert(poly_to_pen_colors.find(poly) == std::cend(poly_to_pen_colors));
        poly_to_pen_colors.emplace(poly, current_color);
        return;
    }

    assert(false); // should have pen or brush exclusively
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

int main(int argc, char* argv[])
{
    using std::cout;
    using std::endl;
    cout << std::boolalpha;

    QApplication app(argc, argv);

    QSvgRenderer renderer;
    const auto load_ok = renderer.load(QString(":map.svg"));
    cout << "renderer " << renderer.isValid() << " " << load_ok << endl;
    assert(renderer.isValid());
    assert(load_ok);

    SvgDumpDevice device;
    QPainter painter(&device);
    renderer.render(&painter);
    cout << "done painting" << endl;

    const auto& engine = device.engine;
    cout << "poly_to_brush_colors " << engine.poly_to_brush_colors.size() << endl;
    cout << "poly_to_pen_colors " << engine.poly_to_pen_colors.size() << endl;

    return 0;
}

