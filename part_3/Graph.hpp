#pragma once

#include "Point.hpp"
#include <vector>


class Graph {
public:

    void newGraph(const std::vector<Point>& points);

    bool addPoint(const Point& p);
    bool removePoint(const Point& p);

    std::vector<Point> convexHull() const;
    double area() const;

private:
    std::vector<Point> points_;
    std::vector<Point> ComputeConvexHull(std::vector<Point>& pts) const;
    double ComputeArea(const std::vector<Point>& P) const;
    bool hasPoint(const Point& p) const;

};

