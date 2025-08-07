#include "Graph.hpp"
#include <iostream>
#include <vector>
#include <algorithm>
#include <cmath>
#include <sstream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

// static constexpr int BUFFER_SIZE = 1024;
static constexpr int PORT = 9034;

bool recvLine(int fd, std::string& line) {
    line.clear();
    char c;
    while (true) {
        ssize_t n = recv(fd, &c, 1, 0);
        if (n <= 0) return false; // Error or connection closed
        if (c == '\n') break;
        if (c != '\r') line.push_back(c);
    }
    return true; // Successfully received a line
}

void sendAll(int fd, const std::string& message) {
    const char* msg = message.data();
    size_t msgLength = message.size();
    while(msgLength > 0) {
        ssize_t n = send(fd, msg, msgLength, 0);
        if (n < 0) {
            std::cerr << "Error sending data\n";
            return; // Handle error appropriately
        }
        msg += n;
        msgLength -= n;
    }
}


int handleClient(int clientSocket) {
    std::ios::sync_with_stdio(false);
    std::cin.tie(nullptr);

    Graph graph;
    std::string line;

    while (recvLine(clientSocket, line)) {
        if (line.empty()) continue;
        std::istringstream in(line);
        std::string cmd;
        in >> cmd;

        if (cmd == "NewGraph") {
            int n; in >> n;
            std::vector<Point> pts;
            bool readOk = true;

            for (int i = 0; i < n; ++i) {
                if (!recvLine(clientSocket, line)){
                    readOk = false;
                    break;
                } 
                if (line.empty()) { i--; continue; }
                double x, y; char comma;
                std::istringstream ptin(line);
                if (!(ptin >> x >> comma >> y) || comma != ',') {
                    std::ostringstream err;
                    err << "Invalid point format: " << line << "\n";
                    readOk = false;
                    sendAll(clientSocket, err.str());
                    break;
                }
                pts.emplace_back(Point{x,y});
            }
            if(readOk){
                graph.newGraph(pts);
            }
            continue;

        } else if (cmd == "CH") {
            auto hull = graph.convexHull();
            double area = graph.area();
            std::ostringstream out;
            for (auto &p : hull)
                out << p.x << "," << p.y << std::endl;
            out << "Area = " << area << std::endl;
            sendAll(clientSocket, out.str());

        } else if (cmd == "NewPoint") {
            double x, y; char comma;
            in >> x >> comma >> y;
            graph.addPoint(Point{x, y});
            std::ostringstream response;
            response << "Point added: " << x << "," << y << "\n";
            sendAll(clientSocket, response.str());

        } else if (cmd == "RemovePoint") {
            double x, y; char comma;
            in >> x >> comma >> y;
            if (comma != ',') {
                std::ostringstream err;
                err << "Invalid point format: " << line << "\n";
                sendAll(clientSocket, err.str());
                break;
            }
            graph.removePoint(Point{x, y});
            std::ostringstream response;
            response << "Point removed: " << x << "," << y << "\n";
            sendAll(clientSocket, response.str());
            

        } else if (cmd == "AddEdge") {
            double x1, y1, x2, y2; char comma1, comma2;
            in >> x1 >> comma1 >> y1 >> x2 >> comma2 >> y2;
            if (comma1 != ',' || comma2 != ',') {
                std::ostringstream err;
                err << "Invalid edge format: " << line << "\n";
                sendAll(clientSocket, err.str());
                continue;
            }
            graph.addEdge(Point{x1, y1}, Point{x2, y2});
            std::ostringstream response;
            response << "Edge added: (" << x1 << "," << y1 << ") - (" << x2 << "," << y2 << ")\n";
            sendAll(clientSocket, response.str());

        } else if (cmd == "RemoveEdge") {
            double x1, y1, x2, y2; char comma1, comma2;
            in >> x1 >> comma1 >> y1 >> x2 >> comma2 >> y2;
            if (comma1 != ',' || comma2 != ',') {
                std::ostringstream err;
                err << "Invalid edge format: " << line << "\n";
                sendAll(clientSocket, err.str());
                continue;
            }
            graph.removeEdge(Point{x1, y1}, Point{x2, y2});
            std::ostringstream response;
            response << "Edge removed: (" << x1 << "," << y1 << ") - (" << x2 << "," << y2 << ")\n";
            sendAll(clientSocket, response.str());

        } else {
            std::cerr << "Unknown command: " << cmd << "\n";
            sendAll(clientSocket, "Unknown command\n");
            break;
        }
    }
    return 0;
}

int main() {

    std::cout << "Starting Graph server on port " << PORT << "...\n";

    int listenfd = socket(AF_INET, SOCK_STREAM, 0);
    if (listenfd < 0) {
        std::cerr << "Error creating socket\n";
        return 1;  
    }
    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(PORT);
    if (bind(listenfd, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        std::cerr << "Error binding socket\n";
        close(listenfd);
        return 1;
    }

    if (listen(listenfd, SOMAXCONN) < 0) {
        std::cerr << "Error listening on socket\n";
        close(listenfd);
        return 1;
    }
    
    while(true) {
        int clientSocket = accept(listenfd, nullptr, nullptr);
        if (clientSocket < 0) {
            std::cerr << "Error accepting connection\n";
            continue;
        }
        
        std::cout << "Client connected.\n";
        handleClient(clientSocket);
        close(clientSocket);
        std::cout << "Client disconnected.\n";
    }


}