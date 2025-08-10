#include "Graph.hpp"
#include "reactor.hpp"
#include <iostream>
#include <vector>
#include <algorithm>
#include <cmath>
#include <sstream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <unordered_map>


static constexpr int PORT = 9034;
static Graph gGraph;
static void* gReactor = nullptr;

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
            // std::cerr << "Error sending data\n";
            return; // Handle error appropriately
        }
        msg += n;
        msgLength -= n;
    }
}


void onClientReadable(int fd) {

    std::string line;
    if (!recvLine(fd, line)){
        removeFdFromReactor(gReactor, fd);
        close(fd);
        std::cout << "Client disconnected.\n";
    }

    std::string cmd;
    std::istringstream in(line);
    in >> cmd;

    if (cmd == "NewGraph") {
        int n; in >> n;
        std::vector<Point> pts;

        for (int i = 0; i < n; ++i) {
            recvLine(fd, line);
            double x, y; char comma;
            std::istringstream ptin(line);
            ptin >> x >> comma >> y;
            if (ptin.fail() || comma != ',') {
                std::ostringstream err;
                err << "Invalid point format: " << line << "\n";
                sendAll(fd, err.str());
                continue;
            }
            pts.emplace_back(Point{x,y});
        }
        gGraph.newGraph(pts);
        sendAll(fd, "New Graph created\n");

    } else if (cmd == "CH") {
        auto hull = gGraph.convexHull();
        double area = gGraph.area();
        std::ostringstream out;
        for (auto &p : hull)
            out << p.x << "," << p.y << std::endl;
        out << "Area = " << area << std::endl;
        sendAll(fd, out.str());

    } else if (cmd == "NewPoint") {
        double x, y; char comma;
        in >> x >> comma >> y;
        gGraph.addPoint(Point{x, y});
        std::ostringstream response;
        response << "Point added: " << x << "," << y << "\n";
        sendAll(fd, response.str());

    } else if (cmd == "RemovePoint") {
        double x, y; char comma;
        in >> x >> comma >> y;
        gGraph.removePoint(Point{x, y});
        std::ostringstream response;
        response << "Point removed: " << x << "," << y << "\n";
        sendAll(fd, response.str());

    } else if (cmd == "AddEdge") {
        double x1, y1, x2, y2; char comma1, comma2;
        in >> x1 >> comma1 >> y1 >> x2 >> comma2 >> y2;
        gGraph.addEdge(Point{x1, y1}, Point{x2, y2});
        std::ostringstream response;
        response << "Edge added: (" << x1 << "," << y1 << ") - (" << x2 << "," << y2 << ")\n";
        sendAll(fd, response.str());

    } else if (cmd == "RemoveEdge") {
        double x1, y1, x2, y2; char comma1, comma2;
        in >> x1 >> comma1 >> y1 >> x2 >> comma2 >> y2;
        gGraph.removeEdge(Point{x1, y1}, Point{x2, y2});
        std::ostringstream response;
        response << "Edge removed: (" << x1 << "," << y1 << ") - (" << x2 << "," << y2 << ")\n";
        sendAll(fd, response.str());

    } else {
        sendAll(fd, "Unknown command\n");
    }
}

void onAccept(int listenFd){
    int clientSocket = accept(listenFd, nullptr, nullptr);
    if (clientSocket < 0) {
        std::cerr << "Error accepting client connection\n";
        return;
    }
    // Add the client socket to the reactor
    addFdToReactor(gReactor, clientSocket, onClientReadable);
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

    gReactor = startReactor();
    addFdToReactor(gReactor, listenfd, onAccept);

    for(;;) pause(); // Keep the server running

    stopReactor(gReactor);
    close(listenfd);
    return 0;
}