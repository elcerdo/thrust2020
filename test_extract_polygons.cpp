#include "extract_polygons.h"
#include "decompose_polygons.h"

#include <QApplication>

#include <iostream>
#include <iomanip>

void dump_poly_to_colors(const polygons::PolyToColors& poly_to_pen_colors)
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

        /*
        cout << "=================" << endl;

        cxd::ConcavePolygon poly_(poly);
        poly_.convexDecomp();
        cout << poly_.getNumberSubPolys() << endl;
        */
    }
}

void
check_non_null_decomposition(const std::string& map)
{
    using std::cout;
    using std::endl;
    using std::get;

    const auto polys = polygons::extract(map);

    cout << "==================== " << std::quoted(map) << endl;
    cout << "poly_to_pen_colors " << get<0>(polys).size() << endl;
    dump_poly_to_colors(get<0>(polys));

    cout << "poly_to_brush_colors " << get<1>(polys).size() << endl;
    dump_poly_to_colors(get<1>(polys));

    for (const auto& poly_color : get<1>(polys))
    {
        if (!polygons::isForeground(poly_color.second))
            continue;

        const auto subpolys = polygons::decompose(polygons::ensure_cw(poly_color.first), 1e-2);
        cout << "decomp " << poly_color.first.size() << " " << subpolys.size() << " ";
        size_t accum = 0;
        bool first = true;
        for (const auto& subpoly : subpolys)
        {
            accum += subpoly.size();
            cout << (first ? "[" : ", ") << subpoly.size();
            first = false;
        }
        cout << "]" << endl;

        if (subpolys.empty())
            std::exit(1);
    }
}

int main(int argc, char* argv[])
{
    using std::cout;

    cout << std::boolalpha;

    QApplication app(argc, argv);
    check_non_null_decomposition(":/level/map1.svg");
    check_non_null_decomposition(":/level/map2.svg");
    check_non_null_decomposition(":/level/map3.svg");
    check_non_null_decomposition(":/level/map4.svg");
    check_non_null_decomposition(":/level/map5.svg");
    check_non_null_decomposition(":/level/map6.svg");

    return 0;
}

