#include "ConcavePolygon.h"
#include "extract_polygons.h"

#include <QApplication>

#include <iostream>
#include <iomanip>

void dump_poly_to_colors(const PolyToColors& poly_to_pen_colors)
{
    using std::cout;
    using std::endl;

    const auto hasher = poly_to_pen_colors.hash_function();
    for (const auto& pair : poly_to_pen_colors)
    {
        const auto& poly = pair.first;
        const auto& color = pair.second;
        const auto& hash = hasher(poly);
        cout
            << "  poly "
            << std::setw(16) << std::setfill('0') << std::hex << hash << std::dec << " "
            << "(" << color.x << "|" << color.y << "|" << color.z << "|" << color.w << ") "
            << poly.size() << endl;

        for (const auto& point : poly)
            cout << "    " << point.x << " " << point.y << endl;

        cout << "=================" << endl;

        cxd::ConcavePolygon::VertexArray vertices_;
        for (const auto& point : poly)
            vertices_.emplace_back(cxd::Vec2 { point.x, point.y });

        cxd::ConcavePolygon poly_(vertices_);
        poly_.convexDecomp();
        cout << poly_.getNumberSubPolys() << endl;
    }
}

int main(int argc, char* argv[])
{
    using std::cout;
    using std::endl;
    using std::get;

    cout << std::boolalpha;

    QApplication app(argc, argv);

    const auto polys = extract_polygons(":map.svg");

    cout << "poly_to_pen_colors " << get<0>(polys).size() << endl;
    dump_poly_to_colors(get<0>(polys));

    cout << "poly_to_brush_colors " << get<1>(polys).size() << endl;
    dump_poly_to_colors(get<1>(polys));

    return 0;
}

