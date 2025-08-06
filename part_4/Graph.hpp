#pragma once

#include "Point.hpp"
#include <vector>


class Graph {
public:

    void newGraph(const std::vector<Point>& points);

    void addPoint(const Point& p);
    void removePoint(const Point& p);

    std::vector<Point> convexHull() const;
    double area() const;

private:
    std::vector<Point> points_;
    std::vector<Point> ComputeConvexHull(std::vector<Point>& pts) const;
    double ComputeArea(const std::vector<Point>& P) const;

};

