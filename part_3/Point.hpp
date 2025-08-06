#pragma once

struct Point {
    double x;
    double y;

    // For sorting points: left to right, then bottom to top
    bool operator < (const Point& p) const { 
        return x < p.x || (x == p.x && y < p.y);
    }

    bool operator == (const Point& p) const {
        return x == p.x && y == p.y;
    }

};

// Function that tells whether the point `o` is to the left, right, or on the line formed by points `a` and `b`.
inline double cross(const Point& o, const Point& a, const Point& b) {
    return (a.x - o.x) * (b.y - o.y) - (a.y - o.y) * (b.x - o.x);
}

