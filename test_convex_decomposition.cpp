#include "ConcavePolygon.h"

#include <iostream>

template <typename BB>
void
require(const BB cond, const std::string& message)
{
    if (static_cast<bool>(cond))
        throw std::runtime_error(message);
}

using Vertices = cxd::ConcavePolygon::Vertices;

void
run_convex_decomp(const Vertices& vertices)
{
    cout << "====================" << endl;
    for (const auto& point : vertices)
        cout << point.x << " " << point.y << endl;

    cxd::ConcavePolygon poly(vertices);
    poly.convexDecomp();

    for (int kk=0, kk_max=poly.getNumberSubPolys(); kk<kk_max; kk++)
    {
        const auto& subpoly = poly.getSubPolygon(kk);
        cout << "** " << kk << "/" << kk_max << " " << subpoly.getNumberSubPolys() << endl;
        for (const auto& point : subpoly.getVertices())
            cout << "  " << point.x << " " << point.y << endl;
    }
}

int main(int argc, char* argv[])
{
    run_convex_decomp(Vertices {
        { -1, -1 },
        { 1, -1 },
        { -.5, -.5 },
        { -1 , 1 }
    });

    run_convex_decomp(Vertices {
        { -1, -1 },
        { 1, -1 },
        { 1, 1 },
        { -1 , 1 }
    });

    run_convex_decomp(Vertices {
        { -1, -1 },
        { 1, -1 },
        { 1, 1 },
        { 0, .5 },
        { -1 , 1 }
    });

    return 0;
}

