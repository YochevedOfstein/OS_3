#include "Point.hpp"
#include <iostream>
#include <vector>
#include <algorithm>
#include <cmath>

using namespace std;

// Monotone‐chain convex hull
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
    for (int i = 0; i < n; ++i) {
        cin >> pts[i].x >> pts[i].y;
    }

    auto hull = convexHull(pts);
    double area = polygonArea(hull);

    cout << "Area = " << area << "\n";
    return 0;
}
