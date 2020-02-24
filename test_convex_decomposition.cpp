#include "ConcavePolygon.h"

#include <iostream>

int main(int argc, char* argv[])
{
    const cxd::ConcavePolygon::VertexArray points = {
        cxd::Vertex {{ -1, -1 }},
        cxd::Vertex {{ 1, -1 }},
        cxd::Vertex {{ -.5, -.5 }},
        cxd::Vertex {{ -1 , 1 }}
    };

    cxd::ConcavePolygon poly(points);
    poly.convexDecomp();

    cout << "subpolys " << poly.getNumberSubPolys() << endl;
    for (int kk=0, kk_max=poly.getNumberSubPolys(); kk<kk_max; kk++)
    {
        const auto& subpoly = poly.getSubPolygon(kk);
        cout << "===== " << subpoly.getNumberSubPolys() << endl;
        for (const auto& point : subpoly.getVertices())
            cout << "  " << point.position.x << " " << point.position.y << endl;
    }

    return 0;
}

