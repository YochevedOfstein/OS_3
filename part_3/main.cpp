#include <iostream>
#include <vector>
#include <algorithm>
#include <cmath>
#include <sstream>
#include "Graph.hpp"

using namespace std;


int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    Graph graph;
    string line;

    while (getline(cin, line)) {
        if (line.empty()) continue;
        istringstream in(line);
        string cmd;
        in >> cmd;

        if (cmd == "Newgraph") {
            int n; in >> n;
            vector<Point> pts;

            for (int i = 0; i < n; ++i) {
                if (!getline(cin, line)) break;
                if (line.empty()) { i--; continue; }
                double x, y; char comma;
                istringstream ptin(line);
                if (!(ptin >> x >> comma >> y) || comma != ',') {
                    cerr << "Invalid point format: " << line << "\n";
                    break;
                }
                pts.emplace_back(Point{x,y});
            }
            graph.newGraph(pts);

        } else if (cmd == "CH") {
            auto hull = graph.convexHull();
            double area = graph.area();
            for (auto &p : hull)
                cout << p.x << "," << p.y << std::endl;
            cout << "Area = " << area << std::endl;

        } else if (cmd == "Newpoint") {
            double x, y; char comma;
            in >> x >> comma >> y;
            graph.addPoint(Point{x, y});

        } else if (cmd == "Removepoint") {
            double x, y; char comma;
            in >> x >> comma >> y;
            graph.removePoint(Point{x, y});

        } else {
            cerr << "Unknown command: " << cmd << "\n";
            break;
        }
    }
    return 0;
}
