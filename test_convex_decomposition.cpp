#include "ConcavePolygon.h"

#include <acd2d_core.h>
#include <acd2d_concavity.h>

#include <iostream>

template <typename BB>
void
require(const BB cond, const std::string& message)
{
    if (!static_cast<bool>(cond))
        throw std::runtime_error(message);
}

using Vertices = cxd::ConcavePolygon::Vertices;


class Prout : public acd2d::IConcavityMeasure
{
    public :

        acd2d::cd_vertex * findMaxNotch(acd2d::cd_vertex * v1, acd2d::cd_vertex * v2);

    private:

        // compute distance from a point, qp, to a segment p1->p2
        double findDist(const acd2d::Vector2d& n, const acd2d::Point2d& p, const acd2d::Point2d& qp);
};

acd2d::cd_vertex * Prout::findMaxNotch(acd2d::cd_vertex * v1, acd2d::cd_vertex * v2)
{
    using namespace acd2d;

    cd_vertex * r=NULL;
    cd_vertex * ptr=v1->getNext();
    Vector2d v=(v2->getPos()-v1->getPos());
    Vector2d n(-v[1],v[0]);
    double norm=n.norm();
    if( norm!=0 ) n=n/norm;

    do{
        double c;
        if( norm!=0) c=findDist(n,v1->getPos(),ptr->getPos());
        else c=(v1->getPos()-ptr->getPos()).norm();
        ptr->setConcavity(c);
        if( r==NULL ) r=ptr;
        else if( r->getConcavity()<c ){ r=ptr; }
        ptr=ptr->getNext();
    }while( ptr!=v2 );
    v1->setConcavity(0);
    v2->setConcavity(0);
    return r;
}

double Prout::findDist(const acd2d::Vector2d& n, const acd2d::Point2d& p, const acd2d::Point2d& qp)
{
    return (qp-p)*n;
}

void
run_acd2d(const Vertices& vertices)
{
    using Vertex = acd2d::cd_vertex;
    using Poly = acd2d::cd_poly;
    using Polygon = acd2d::cd_polygon;
    using Decomposition = acd2d::cd_2d;
    //using Measure = acd2d::HybridMeasurement1;
    using Measure = Prout;

    cout << "==================== acd2d" << endl;

    Polygon polygon;
    {
        Poly poly(Poly::POUT);
        poly.beginPoly();
        for (const auto& point : vertices)
            poly.addVertex(point.x, point.y);
        poly.endPoly();
        //poly.scale(1);
        cout << poly.getSize() << endl;
        polygon.emplace_back(poly);
        //polygon.buildDependency();
    }
    require(polygon.valid(), "invalid polygon");

    Decomposition decomp;
    decomp.addPolygon(polygon);

    Measure measure;

    cout << decomp.getTodoList().size() << " " << decomp.getDoneList().size() << endl;
    decomp.decomposeAll(.1, &measure);
    cout << decomp.getTodoList().size() << " " << decomp.getDoneList().size() << endl;

    for (const Polygon& subpolygon : decomp.getDoneList())
    {
        cout << "** " << subpolygon.size() << " ";
        cout.flush();
        assert(subpolygon.size() == 1);
        const auto& subpoly = subpolygon.front();
        cout << subpoly.getSize() << endl;


        const auto head = subpoly.getHead();
        auto ptr = head;
        do {
            assert(ptr);
            const auto& position = ptr->getPos();
            cout << "  " << position[0] << " " << position[1] << endl;
            ptr=ptr->getNext();
        }
        while(ptr!=head);

    }
}


void
run_convex_decomp(const Vertices& vertices)
{
    cout << "==================== convex decomp" << endl;
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

void
run_all(const Vertices& vertices)
{
    run_convex_decomp(vertices);
    run_acd2d(vertices);
}

int main(int argc, char* argv[])
{
    std::cout << std::boolalpha;

    run_all(Vertices {
        { -1, -1 },
        { 1, -1 },
        { -.5, -.5 },
        { -1 , 1 }
    });

    run_all(Vertices {
        { -1, -1 },
        { 1, -1 },
        { 1, 1 },
        { -1 , 1 }
    });

    run_all(Vertices {
        { -1, -1 },
        { 1, -1 },
        { 1, 1 },
        { 0, .5 },
        { -1 , 1 }
    });

    return 0;
}

