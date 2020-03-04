#include "decompose_polygons.h"

#include <iostream>
#include <vector>

template <typename BB>
void
require(const BB cond, const std::string& message)
{
    if (!static_cast<bool>(cond))
        throw std::runtime_error(message);
}

void
run_acd2d(const polygons::Poly& poly)
{
    using std::cout;
    using std::endl;

    cout << "==================== acd2d" << endl;

    cout << "II " << poly.size() << endl;
    for (const auto& point : poly)
        cout << "  " << point.x << " " << point.y << endl;

    const auto& poly_ = polygons::ensure_cw(poly);

    cout << "NN " << poly_.size() << endl;
    for (const auto& point : poly_)
        cout << "  " << point.x << " " << point.y << endl;

    const auto& subpolys = polygons::decompose(poly_, 1e-3);
    for (const auto& subpoly : subpolys)
    {
        cout << "OO " << subpoly.size() << endl;
        for (const auto& point : subpoly)
            cout << "  " << point.x << " " << point.y << endl;
    }
}

int main(int argc, char* argv[])
{
    std::cout << std::boolalpha;

    using polygons::Poly;

    run_acd2d(Poly {
        { -1, -1 },
        { 1, -1 },
        { -.5, -.5 },
        { -1 , 1 }
    });

    run_acd2d(Poly {
        { -1, -1 },
        { 1, -1 },
        { 1, 1 },
        { -1 , 1 }
    });

    run_acd2d(Poly {
        { 1, -1 },
        { -1, -1 },
        { -1 , 1 },
        { 1, 1 },
    });

    run_acd2d(Poly {
        { -1, -1 },
        { 1, -1 },
        { 1, 1 },
        { 0, .5 },
        { -1 , 1 }
    });

    return 0;
}

