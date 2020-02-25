#include "decompose_polygons.h"

#include <acd2d_core.h>
#include <acd2d_concavity.h>
#include <acd2d_edge_visibility.h>

class StraightMesure : public acd2d::IConcavityMeasure
{
    public:
        acd2d::cd_vertex* findMaxNotch(acd2d::cd_vertex* v1, acd2d::cd_vertex* v2);

    private:
        double findDist(const acd2d::Vector2d& n, const acd2d::Point2d& p, const acd2d::Point2d& qp);
};

acd2d::cd_vertex* StraightMesure::findMaxNotch(acd2d::cd_vertex* v1, acd2d::cd_vertex* v2)
{
    using namespace acd2d;

    cd_vertex* r = NULL;
    cd_vertex* ptr = v1->getNext();
    Vector2d v = v2->getPos() - v1->getPos();
    Vector2d n(-v[1],v[0]);
    double norm = n.norm();
    if (norm != 0) n = n/norm;
    do
    {
        double c;
        if (norm != 0) c = findDist(n, v1->getPos(), ptr->getPos());
        else c = (v1->getPos() - ptr->getPos()).norm();
        ptr->setConcavity(c);
        if (r==NULL) r = ptr;
        else if (r->getConcavity() < c) r = ptr;
        ptr = ptr->getNext();
    } while (ptr != v2);
    v1->setConcavity(0);
    v2->setConcavity(0);
    return r;
}

double StraightMesure::findDist(const acd2d::Vector2d& n, const acd2d::Point2d& p, const acd2d::Point2d& qp)
{
    return (qp-p)*n;
}

///////////////////////////////////////////////////////////////////////////////

std::list<polygons::Poly>
polygons::decompose(const polygons::Poly& vertices, const double margin)
{
    using Poly = acd2d::cd_poly;
    using Polygon = acd2d::cd_polygon;
    using Decomposition = acd2d::cd_2d;
    //using Measure = acd2d::HybridMeasurement1;
    using Measure = StraightMesure;
    using Scalar = decltype(b2Vec2::x);

    Polygon polygon;
    {
        Poly poly(Poly::POUT);
        poly.beginPoly();
        for (const auto& point : vertices)
            poly.addVertex(point.x, point.y);
        poly.endPoly();
        //poly.scale(1);
        polygon.emplace_back(poly);
        //polygon.buildDependency();
    }
    assert(polygon.valid());

    Decomposition decomp;
    decomp.addPolygon(polygon);

    Measure measure;
    decomp.decomposeAll(margin, &measure);
    assert(decomp.getTodoList().empty());

    std::list<polygons::Poly> subpolys;
    for (const Polygon& subpolygon : decomp.getDoneList())
    {
        assert(subpolygon.size() == 1);
        const auto& subpoly = subpolygon.front();

        polygons::Poly subpoly_;
        const auto head = subpoly.getHead();
        auto ptr = head;
        do
        {
            assert(ptr);
            const auto& position = ptr->getPos();
            subpoly_.emplace_back(b2Vec2 { static_cast<Scalar>(position[0]), static_cast<Scalar>(position[1]) });
            ptr=ptr->getNext();
        }
        while (ptr != head);

        subpolys.emplace_back(subpoly_);
    }

    return subpolys;
}
