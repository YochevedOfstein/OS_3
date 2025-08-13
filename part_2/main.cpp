#include "Point.hpp"
#include <iostream>
#include <vector>
#include <algorithm>
#include <cmath>
#include <deque>
#include <list>
#include <chrono>

using namespace std;
using HighResClock = std::chrono::high_resolution_clock;
using TimePoint = HighResClock::time_point;

// Original Convex hull
vector<Point> convexHull(const vector<Point>& pts_sorted) {
    const auto& pts = pts_sorted;
    int n = pts.size(), k = 0;
    if (n <= 1) return pts;

    vector<Point> H(2*n);
    // lower
    for (int i = 0; i < n; ++i) {
        while (k >= 2 && cross(H[k-2], H[k-1], pts[i]) <= 0) k--;
        H[k++] = pts[i];
    }
    // upper
    for (int i = n-2, t = k+1; i >= 0; --i) {
        while (k >= t && cross(H[k-2], H[k-1], pts[i]) <= 0) k--;
        H[k++] = pts[i];
    }
    H.resize(k-1);
    return H;
}

// Deque-based convex hull
vector<Point> convexHullDeque(const vector<Point>& points_sorted) {
    const auto& points = points_sorted;
    int n = points.size();
    if (n <= 1) return points;

    deque<Point> H;

    for (const auto& p : points) {
        while (H.size() >= 2 && cross(H[H.size()-2], H[H.size()-1], p) <= 0)
            H.pop_back();
        H.push_back(p);
    }

    size_t t = H.size() + 1;
    for (int i = n - 2; i >= 0; --i) {
        const auto& p = points[i];
        while (H.size() >= t && cross(H[H.size()-2], H[H.size()-1], p) <= 0)
            H.pop_back();
        H.push_back(p);
    }
    H.pop_back();
    return vector<Point>(H.begin(), H.end());
}

// List-based convex hull
vector<Point> convexHullList(const vector<Point>& points_sorted) {
    const auto& points = points_sorted;
    if (points.size() <= 1) return points;

    list<Point> H;

    // lower hull
    for (const auto& p : points) {
        while (H.size() >= 2) {
            auto it_last   = prev(H.end());
            auto it_second = prev(it_last);
            if (cross(*it_second, *it_last, p) <= 0) H.pop_back();
            else break;
        }
        H.push_back(p);
    }

    // upper hull
    size_t t = H.size() + 1;
    for (int i = (int)points.size() - 2; i >= 0; --i) {
        const auto& p = points[i];
        while (H.size() >= t) {
            auto it_last   = prev(H.end());
            auto it_second = prev(it_last);
            if (cross(*it_second, *it_last, p) <= 0) H.pop_back();
            else break;
        }
        H.push_back(p);
    }

    // remove the duplicate of the first point
    H.pop_back();

    return vector<Point>(H.begin(), H.end());
}

// Compute polygon area via the shoelace formula
double polygonArea(const vector<Point>& P) {
    double area = 0;
    int m = P.size();
    for (int i = 0; i < m; ++i) {
        int j = (i+1) % m;
        area += P[i].x * P[j].y - P[j].x * P[i].y;
    }
    return fabs(area) * 0.5;
}

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    // Read input
    int n;
    if (!(cin >> n)) return 0;
    vector<Point> pts(n);
    char comma;

    for (int i = 0; i < n; ++i) {
        cin >> pts[i].x >> comma >> pts[i].y;
    }

    auto sorted = pts;
    sort(sorted.begin(), sorted.end());

    TimePoint t1 = HighResClock::now();
    auto hull = convexHull(sorted);  // Original convex hull
    TimePoint t2 = HighResClock::now();

    TimePoint t3 = HighResClock::now();
    auto hull1 = convexHullList(sorted);   // List-based convex hull
    TimePoint t4 = HighResClock::now();

    TimePoint t5 = HighResClock::now();
    auto hull2 = convexHullDeque(sorted);  // Deque-based convex hull
    TimePoint t6 = HighResClock::now();

    // Compute area once (they should match)
    double area = polygonArea(hull1);

    cout << "Area = " << area << "\n";
    
    double dt1 = chrono::duration<double, milli>(t2 - t1).count();
    double dt2 = chrono::duration<double, milli>(t4 - t3).count();
    double dt3 = chrono::duration<double, milli>(t6 - t5).count();

    cout << "Time for original convex hull: " << dt1 << " ms\n";
    cout << "Time for list-based convex hull: " << dt2 << " ms\n";
    cout << "Time for deque-based convex hull: " << dt3 << " ms\n";

    return 0;

}
