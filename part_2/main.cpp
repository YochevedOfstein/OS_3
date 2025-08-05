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
vector<Point> convexHull(vector<Point>& pts) {
    sort(pts.begin(), pts.end());           // uses Point::operator<
    int n = pts.size(), k = 0;
    if (n <= 1) return pts;

    vector<Point> H(2*n);
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

// Deque-based convex hull
vector<Point> convexHullDeque(vector<Point>& points) {
    sort(points.begin(), points.end()); 
    int n = points.size();
    if (n <= 1) return points;

    deque<Point> H;

    // Build lower hull
    for(auto& p : points) {
        while (H.size() >= 2 && cross(H[H.size()-2], H[H.size()-1], p) <= 0) {
            H.pop_back();
        }
        H.push_back(p);
    }

    // Build upper hull
    auto t = H.size() + 1;
    for(int i = n - 2; i >= 0; --i) {
        auto& p = points[i];
        while (H.size() >= t && cross(H[H.size()-2], H[H.size()-1], p) <= 0) {
            H.pop_back();
        }
        H.push_back(p);
    }
    H.pop_back(); // Remove the last point as it is the same as the first one

    return vector<Point>(H.begin(), H.end());
}

// List-based convex hull
vector<Point> convexHullList(vector<Point>& points) {
    sort(points.begin(), points.end());        
    int n = points.size();
    if (n <= 1) return points;

    list<Point> H;

    // Build lower hull
    for(auto& p : points) {
        auto it_last = prev(H.end(), 1);
        auto it_second = prev(H.end(), 2);
        while (H.size() >= 2 && cross(*it_second, *it_last, p) <= 0) {
            H.pop_back();
            it_last = prev(H.end(), 1);
            it_second = prev(H.end(), 2);
        }
        H.push_back(p);
    }

    // Build upper hull
    auto t = H.size() + 1;
    for(int i = n - 2; i >= 0; --i) {
        auto& p = points[i];
        auto it_last = prev(H.end(), 1);
        auto it_second = prev(H.end(), 2);
        while (H.size() >= t && cross(*it_second, *it_last, p) <= 0) {
            H.pop_back();
            it_last = prev(H.end(), 1);
            it_second = prev(H.end(), 2);
        }
        H.push_back(p);
    }

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

    TimePoint t1 = HighResClock::now();
    auto hull = convexHull(pts);  // Original convex hull
    TimePoint t2 = HighResClock::now();

    TimePoint t3 = HighResClock::now();
    auto hull1 = convexHullList(pts);   // List-based convex hull
    TimePoint t4 = HighResClock::now();

    TimePoint t5 = HighResClock::now();
    auto hull2 = convexHullDeque(pts);  // Deque-based convex hull
    TimePoint t6 = HighResClock::now();

    // Compute area once (they should match)
    double area = polygonArea(hull1);

    // Report
    cout << "Area: " << area << "\n";

    // long dt1 = chrono::duration_cast<chrono::milliseconds>(t2 - t1).count();
    // long dt2 = chrono::duration_cast<chrono::milliseconds>(t4 - t3).count();
    // long dt3 = chrono::duration_cast<chrono::milliseconds>(t6 - t5).count();

    double dt1 = chrono::duration<double, milli>(t2 - t1).count();
    double dt2 = chrono::duration<double, milli>(t4 - t3).count();
    double dt3 = chrono::duration<double, milli>(t6 - t5).count();

    cout << "Time for original convex hull: " << dt1 << " ms\n";
    cout << "Time for list-based convex hull: " << dt2 << " ms\n";
    cout << "Time for deque-based convex hull: " << dt3 << " ms\n";

    // while(cin >> n) {
    //     if (n <= 0) break; // Exit on non-positive input
    //     vector<Point> pts(n);
    //     for (int i = 0; i < n; ++i) {
    //         cin >> pts[i].x >> comma >> pts[i].y;
    //     }
    //     auto hull = convexHull(pts);  // Original convex hull
    //     auto hull1 = convexHullList(pts);   // List-based convex hull
    //     auto hull2 = convexHullDeque(pts);  // Deque-based convex hull
    //     double area = polygonArea(hull);

    //     cout << "Area = " << area << "\n";
    // }



    return 0;

}
