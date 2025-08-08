#pragma once

#include "Point.hpp"
#include <vector>


class Graph {
public:

    void newGraph(const std::vector<Point>& points);

    void addPoint(const Point& p);
    void removePoint(const Point& p);

    void addEdge(const Point& p1, const Point& p2);
    void removeEdge(const Point& p1, const Point& p2);

    std::vector<Point> convexHull() const;
    double area() const;

    const std::vector<Point>& getPoints() const { return points_; }
    const std::vector<std::pair<Point, Point>>& getEdges() const { return edges_; }

private:
    std::vector<Point> points_;
    std::vector<std::pair<Point, Point>> edges_;
    std::vector<Point> ComputeConvexHull(std::vector<Point>& pts) const;
    double ComputeArea(const std::vector<Point>& P) const;

};
