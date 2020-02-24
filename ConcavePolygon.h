#pragma once

#include <Box2D/Common/b2Math.h>
#include <vector>
#include <cmath>
#include <map>
#include <iostream>

using std::cout;
using std::endl;

namespace cxd
{

    float b2Square(const b2Vec2& xx)
    {
        return b2Dot(xx, xx);
    }

    float b2Orient(const b2Vec2& v0, const b2Vec2& v1, const b2Vec2& v2)
    {
        const auto edge0 = v1 - v0;
        const auto edge1 = v2 - v0;
        return b2Cross(edge0, edge1);
    }

    using Vector = b2Vec2;
    using Segment = std::tuple<Vector, Vector>;

    Vector direction(const Segment& segment)
    {
        using std::get;
        return get<1>(segment) - get<0>(segment);
    }

    std::pair<bool, Vector> intersects(const Segment& s1, const Segment& s2)
    {
        using std::get;
        const float TOLERANCE = 1e-2;

        Vector p1 = get<0>(s1);
        Vector p2 = get<0>(s2);
        Vector d1 = direction(s1);
        Vector d2 = direction(s2);

        if(std::abs(b2Cross(d1, d2)) < 1e-30)
           return {false, {0.0f, 0.0f}};

        float t1 = b2Cross(p2 - p1, d2) / b2Cross(d1, d2);

        if((t1 < (0.0f - TOLERANCE)) || (t1 > (1.0f + TOLERANCE)))
            return {false, {0.0f, 0.0f}};

        Vector pIntersect = p1 + d1 * t1;

        float t2 = b2Dot(pIntersect - p2, get<1>(s2) - p2);

        if(t2 < (0.0f-TOLERANCE) || t2 / b2Square(get<1>(s2) - p2) >= 1.0f - TOLERANCE)
            return {false, {0.0f, 0.0f}};

        return {true, pIntersect};
    }

class ConcavePolygon
{
    public:
    typedef std::vector<Vector> VertexArray;
    typedef std::vector<ConcavePolygon> PolygonArray;

    protected:
    VertexArray vertices;
    PolygonArray subPolygons;


    static int mod(int x, int m)
    {
        int r = x % m;
        return r < 0 ? r + m : r;
    }

    static void flipPolygon(VertexArray & _verts)
    {
        int iMax = _verts.size()/2;

        if(_verts.size() % 2 != 0)
            iMax += 1;

        for(int i=1; i<iMax; ++i)
        {
            std::swap(_verts[i], _verts[_verts.size()-i]);
        }
    }

    static bool checkIfRightHanded(const VertexArray& _verts)
    {
        if(_verts.size() < 3)
            return false;

        float signedArea = 0.0f;

        for(unsigned int i=0; i<_verts.size(); ++i)
            signedArea += b2Cross(_verts[i], _verts[mod(i+1, _verts.size())]);

        if(signedArea < 0.0f)
            return true;

        return false;
    }

    static bool isVertexInCone(const Segment& ls1,
                        const Segment& ls2,
                        const Vector& origin,
                        const Vector& vert)
    {
        Vector relativePos = vert - origin;

        float ls1Product = b2Cross(relativePos, direction(ls1));
        float ls2Product = b2Cross(relativePos, direction(ls2));

        if(ls1Product < 0.0f && ls2Product > 0.0f)
            return true;

        return false;
    }

    typedef std::vector<int > IntArray;

    IntArray findVerticesInCone(const Segment& ls1,
                                const Segment& ls2,
                                const Vector& origin,
                                const VertexArray& inputVerts)
    {
        IntArray result;

        for(unsigned int i=0; i<inputVerts.size(); ++i)
        {
            if(isVertexInCone(ls1, ls2, origin, inputVerts[i]))
                result.push_back(i);
        }
        return result;
    }

    static bool checkVisibility(const Vector& originalPosition,
                         const Vector& vert,
                         const VertexArray& polygonVertices)
    {
        const Segment ls { originalPosition, vert };
        VertexIntMap intersectingVerts = verticesAlongLineSegment(ls, polygonVertices);

        //std::cout << intersectingVerts.size() << " intverts\n";

        if(intersectingVerts.size() > 3)
            return false;

        return true;
    }


    int getBestVertexToConnect(IntArray const & indices,
                               VertexArray const & polygonVertices,
                               Vector const & origin)
    {
        if(indices.size()==1)
        {
            if(checkVisibility(origin, polygonVertices[indices[0]], polygonVertices))
                return indices[0];
        }
        else if(indices.size() > 1)
        {
            for(unsigned int i=0; i<indices.size(); ++i)
            {
                const auto& index = indices[i];
                const auto& vertSize = polygonVertices.size();

                const auto& prevVert = polygonVertices[mod(index-1, vertSize)];
                const auto& currVert = polygonVertices[index];
                const auto& nextVert = polygonVertices[mod(index+1, vertSize)];

                const Segment ls1 { prevVert, currVert };
                const Segment ls2 { nextVert, currVert };

                if((b2Orient(prevVert, currVert, nextVert) < 0.0f) &&
                   isVertexInCone(ls1, ls2, polygonVertices[index], origin) &&
                   checkVisibility(origin, polygonVertices[index], polygonVertices))
                    return index;
            }

            for(unsigned int i=0; i<indices.size(); ++i)
            {
                const auto& index = indices[i];
                const auto& vertSize = polygonVertices.size();

                const auto& prevVert = polygonVertices[mod(index-1, vertSize)];
                const auto& currVert = polygonVertices[index];
                const auto& nextVert = polygonVertices[mod(index+1, vertSize)];

                const Segment ls1 { prevVert, currVert };
                const Segment ls2 { nextVert, currVert };

                if((b2Orient(prevVert, currVert, nextVert) < 0.0f) &&
                   checkVisibility(origin, polygonVertices[index], polygonVertices))
                    return index;
            }


            float minDistance = 1e+15;
            int closest = indices[0];
            for(unsigned int i=0; i<indices.size(); ++i)
            {
                int index = indices[i];
                float currDistance = b2Square(polygonVertices[index]- origin);
                if(currDistance < minDistance)
                {
                    minDistance = currDistance;
                    closest = index;
                }
            }

            return closest;
        }


        return -1;
    }

    void convexDecomp(VertexArray const & _vertices)
    {
        if(subPolygons.size() > 0)
        {
            return;
        }

        int reflexIndex = findFirstReflexVertex(_vertices);
        cout << "reflexIndex " << reflexIndex << endl;
        if(reflexIndex == -1)
            return;

        const auto& prevVertPos = _vertices[mod(reflexIndex-1, _vertices.size())];
        const auto& currVertPos = _vertices[reflexIndex];
        const auto& nextVertPos = _vertices[mod(reflexIndex+1, _vertices.size())];

        const Segment ls1 { prevVertPos, currVertPos };
        const Segment ls2 { nextVertPos, currVertPos };

        IntArray vertsInCone = findVerticesInCone(ls1, ls2, currVertPos, _vertices);

        int bestVert = -1;

        if(vertsInCone.size() > 0)
        {
            bestVert = getBestVertexToConnect(vertsInCone, _vertices, currVertPos);
            if(bestVert != -1)
            {
                const Segment newLine { currVertPos, _vertices[bestVert] };

                slicePolygon(newLine);
            }
        }
        if(vertsInCone.size() == 0 || bestVert == -1)
        {
            const Segment newLine { currVertPos, (direction(ls1) + direction(ls2)) * 1e+10 };
            slicePolygon(newLine);
        }

        for(unsigned int i=0; i<subPolygons.size(); ++i)
        {
            subPolygons[i].convexDecomp();
        }
    }

    int findFirstReflexVertex(VertexArray const & _vertices)
    {
        for(unsigned int i=0; i<_vertices.size(); ++i)
        {
            float handedness = b2Orient(_vertices[mod(i-1, _vertices.size())],
                                                     _vertices[i],
                                                     _vertices[mod(i+1, _vertices.size())]);
            if(handedness < 0.0f)
                return i;
        }

        return -1;
    }

    void flipPolygon()
    {
        flipPolygon(vertices);
    }

    using VertexIntMap = std::map<int, Vector>;

    VertexIntMap cullByDistance(const VertexIntMap& input,
                                const Vector& origin,
                                const int& maxVertsToKeep)
    {
        if(maxVertsToKeep >= (int)input.size())
            return input;

        struct SliceVertex
        {
            Vector position;
            int index;
            float distanceToSlice;
        };

        std::vector<SliceVertex > sliceVertices;

        for(auto it = input.begin(); it != input.end(); ++it)
        {
            const SliceVertex vert { it->second, it->first, b2Square(it->second- origin) };
            sliceVertices.push_back(vert);
        }

        for(unsigned int i=1; i<sliceVertices.size(); ++i)
            for(unsigned int j=i; j > 0 && sliceVertices[j].distanceToSlice < sliceVertices[j-1].distanceToSlice; --j)
                std::swap(sliceVertices[j], sliceVertices[j-1]);

        sliceVertices.erase(sliceVertices.begin()+maxVertsToKeep, sliceVertices.end());

        for(unsigned int i=1; i<sliceVertices.size(); ++i)
            for(unsigned int j=i; j > 0 && sliceVertices[j].index < sliceVertices[j-1].index; --j)
                std::swap(sliceVertices[j], sliceVertices[j-1]);

        VertexIntMap result;
        for(unsigned int i=0; i<sliceVertices.size(); ++i)
        {
            result.emplace(sliceVertices[i].index, sliceVertices[i].position);
        }

        return result;
    }

    static VertexIntMap verticesAlongLineSegment(const Segment& segment, const VertexArray& _vertices)
    {
        VertexIntMap result;

        for(unsigned int i=0; i<_vertices.size(); ++i)
        {
            const Segment tempSegment { _vertices[i], _vertices[mod(i+1, _vertices.size())] };

            const auto intersectionResult = intersects(segment, tempSegment);

            if(intersectionResult.first == true)
            {
                result.emplace(i, intersectionResult.second);
            }
        }

        return result;
    }

public:
    ConcavePolygon(VertexArray const & _vertices) : vertices{_vertices}
    {
        if(vertices.size() > 2)
            if(checkIfRightHanded() == false)
                flipPolygon();
    }
    ConcavePolygon() {}

    bool checkIfRightHanded()
    {
        return checkIfRightHanded(vertices);
    }

    void slicePolygon(int vertex1, int vertex2)
    {
        if(vertex1 == vertex2 ||
           vertex2 == vertex1+1 ||
           vertex2 == vertex1-1)
            return;

        if(vertex1 > vertex2)
            std::swap(vertex1, vertex2);

        VertexArray returnVerts;
        VertexArray newVerts;
        for(int i=0; i<(int)vertices.size(); ++i)
        {
            if(i==vertex1 || i==vertex2)
            {
                returnVerts.push_back(vertices[i]);
                newVerts.push_back(vertices[i]);
            }
            else if(i > vertex1 && i <vertex2)
                returnVerts.push_back(vertices[i]);
            else
                newVerts.push_back(vertices[i]);
        }

        subPolygons.push_back(ConcavePolygon(returnVerts));
        subPolygons.push_back(ConcavePolygon(newVerts));
    }

    void slicePolygon(const Segment& segment)
    {
        if(subPolygons.size() > 0)
        {
            subPolygons[0].slicePolygon(segment);
            subPolygons[1].slicePolygon(segment);
            return;
        }

        const float TOLERANCE = 1e-5;

        VertexIntMap slicedVertices = verticesAlongLineSegment(segment, vertices);
        slicedVertices = cullByDistance(slicedVertices, std::get<0>(segment), 2);

        if(slicedVertices.size() < 2)
            return;

        VertexArray leftVerts;
        VertexArray rightVerts;

        for(int i=0; i<(int)vertices.size(); ++i)
        {
            const auto relativePosition = vertices[i] - std::get<0>(segment);

            auto it = slicedVertices.begin();

            float perpDistance = std::abs(b2Cross(relativePosition, direction(segment)));
            if(perpDistance > TOLERANCE)
            {
                //std::cout << relCrossProd << ", i: " << i << "\n";
                if((i > it->first) && (i <= (++it)->first))
                {
                    leftVerts.push_back(vertices[i]);
                    //std::cout << i << " leftVertAdded\n";
                }
                else
                {
                    rightVerts.push_back(vertices[i]);
                    //std::cout << i << " rightVertAdded\n";
                }

            }

            if(slicedVertices.find(i) != slicedVertices.end())
            {
                rightVerts.push_back(slicedVertices[i]);
                leftVerts.push_back(slicedVertices[i]);
            }
        }

        subPolygons.push_back(ConcavePolygon(leftVerts));
        subPolygons.push_back(ConcavePolygon(rightVerts));
    }

    void convexDecomp()
    {
        if(vertices.size() > 3)
            convexDecomp(vertices);
    }

    VertexArray const & getVertices() const
    {
        return vertices;
    }

    ConcavePolygon const & getSubPolygon(int subPolyIndex) const
    {
        if(subPolygons.size() > 0 && subPolyIndex < (int)subPolygons.size())
            return subPolygons[subPolyIndex];

        return *this;
    }

    int getNumberSubPolys() const
    {
        return subPolygons.size();
    }

    void returnLowestLevelPolys(std::vector<ConcavePolygon > & returnArr)
    {
        if(subPolygons.size() > 0)
        {
            subPolygons[0].returnLowestLevelPolys(returnArr);
            subPolygons[1].returnLowestLevelPolys(returnArr);
        }
        else
            returnArr.push_back(*this);
    }

    void reset()
    {
        if(subPolygons.size() > 0)
        {
            subPolygons[0].reset();
            subPolygons[1].reset();
            subPolygons.clear();
        }
    }

    Vector getPoint(unsigned int index) const
    {
        if(index >= 0 && index < vertices.size())
            return vertices[index];

        return {0.0f, 0.0f};
    }

    int getPointCount() const
    {
        return vertices.size();
    }
};

}

