#include "Graph.hpp"
#include <algorithm>
#include <cmath>

void Graph::newGraph(const std::vector<Point>& points) {
    points_ = points;
}

void Graph::addPoint(const Point& p) {
    points_.push_back(p);
}

void Graph::removePoint(const Point& p) {
    points_.erase(std::remove(points_.begin(), points_.end(), p), points_.end());
}

std::vector<Point> Graph::convexHull() const {
    std::vector<Point> pts = points_;
    return ComputeConvexHull(pts);
}

double Graph::area() const {
    auto hull = convexHull();
    return ComputeArea(hull);
}
std::vector<Point> Graph::ComputeConvexHull(std::vector<Point>& pts) const {
    sort(pts.begin(), pts.end());           // uses Point::operator<
    int n = pts.size(), k = 0;
    if (n <= 1) return pts;

    std::vector<Point> H(2*n);
    // Build lower hull
    for (int i = 0; i < n; ++i) {
        while (k >= 2 && cross(H[k-2], H[k-1], pts[i]) <= 0) {
            k--;
        }
        H[k++] = pts[i];
    }
    // Build upper hull
    for (int i = n-2, t = k+1; i >= 0; --i) {
        while (k >= t && cross(H[k-2], H[k-1], pts[i]) <= 0) {
            k--;
        }
        H[k++] = pts[i];
    }
    H.resize(k-1);
    return H;
}

double Graph::ComputeArea(const std::vector<Point>& P) const {
    double area = 0;
    int m = P.size();
    for (int i = 0; i < m; ++i) {
        int j = (i+1) % m;
        area += P[i].x * P[j].y - P[j].x * P[i].y;
    }
    return fabs(area) * 0.5;
}