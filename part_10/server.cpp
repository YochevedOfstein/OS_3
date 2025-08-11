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
#include <mutex>
#include <pthread.h>
#include <signal.h>


static constexpr int PORT = 9034;

Graph graph;
std::mutex graphMutex;

static pthread_mutex_t gMonMtx = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t gMonCv = PTHREAD_COND_INITIALIZER;

static double gLastArea = 0.0;
static bool gHasUpdated = false;
static bool gAtLeast100 = false;
static bool gShuttingDown = false;


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

static void* monitorArea(void*){
    pthread_mutex_lock(&gMonMtx);
    while (!gShuttingDown) {
        while (!gHasUpdated && !gShuttingDown)
            pthread_cond_wait(&gMonCv, &gMonMtx);

        if (gShuttingDown) break;

        double a = gLastArea;
        bool nowAtLeast100 = (a >= 100.0);

        if (nowAtLeast100 && !gAtLeast100) {
            std::cout << "At Least 100 units belongs to CH\n";
            gAtLeast100 = true;
        } else if (!nowAtLeast100 && gAtLeast100) {
            std::cout << "At Least 100 units no longer belongs to CH\n";
            gAtLeast100 = false;
        }

        gHasUpdated = false; // consume update
    }
    pthread_mutex_unlock(&gMonMtx);
    return nullptr;
}


static void* handleClient(int clientSocket) {
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
                std::lock_guard<std::mutex> lock(graphMutex);
                graph.newGraph(pts);
                sendAll(clientSocket, "New graph created\n");
            }
            continue;

        } else if (cmd == "CH") {
            std::vector<Point> hull;
            double area;
            {
                std::lock_guard<std::mutex> lock(graphMutex);
                hull = graph.convexHull();
                area = graph.area();
            }
            std::ostringstream out;
            for (auto &p : hull)
                out << p.x << "," << p.y << std::endl;
            out << "Area = " << area << std::endl;
            pthread_mutex_lock(&gMonMtx);
            gLastArea = area;
            gHasUpdated = true;
            pthread_cond_signal(&gMonCv);
            pthread_mutex_unlock(&gMonMtx);
            sendAll(clientSocket, out.str());

        } else if (cmd == "NewPoint") {
            double x, y; char comma;
            in >> x >> comma >> y;
            {
                std::lock_guard<std::mutex> lock(graphMutex);
                graph.addPoint(Point{x, y});
            }
            std::ostringstream response;
            response << "Point added: " << x << "," << y << "\n";
            sendAll(clientSocket, response.str());

        } else if (cmd == "RemovePoint") {
            double x, y; char comma;
            in >> x >> comma >> y;
            {
                std::lock_guard<std::mutex> lock(graphMutex);
                graph.removePoint(Point{x, y});
            }
            std::ostringstream response;
            response << "Point removed: " << x << "," << y << "\n";
            sendAll(clientSocket, response.str());
            

        } else if (cmd == "AddEdge") {
            double x1, y1, x2, y2; char comma1, comma2;
            in >> x1 >> comma1 >> y1 >> x2 >> comma2 >> y2;
            {
                std::lock_guard<std::mutex> lock(graphMutex);
                 graph.addEdge(Point{x1, y1}, Point{x2, y2});
            }
            std::ostringstream response;
            response << "Edge added: (" << x1 << "," << y1 << ") - (" << x2 << "," << y2 << ")\n";
            sendAll(clientSocket, response.str());

        } else if (cmd == "RemoveEdge") {
            double x1, y1, x2, y2; char comma1, comma2;
            in >> x1 >> comma1 >> y1 >> x2 >> comma2 >> y2;
            {
                std::lock_guard<std::mutex> lock(graphMutex);
                graph.removeEdge(Point{x1, y1}, Point{x2, y2});
            }
            std::ostringstream response;
            response << "Edge removed: (" << x1 << "," << y1 << ") - (" << x2 << "," << y2 << ")\n";
            sendAll(clientSocket, response.str());

        } else {
            sendAll(clientSocket, "Unknown command\n");
            break;
        }
    }
    close(clientSocket);
    return nullptr;
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

    pthread_t monTid;
    pthread_create(&monTid, nullptr, &monitorArea, nullptr);

    pthread_t acceptTid = startProactor(listenfd, &handleClient);
    if (!acceptTid) {
        std::cerr << "Error starting proactor thread\n";
        close(listenfd);
        return 1;
    }

    for(;;){
        pause(); // Wait indefinitely
    }

    // pthread_mutex_lock(&gMonMtx);
    // gShuttingDown = true;
    // pthread_cond_signal(&gMonCv);
    // pthread_mutex_unlock(&gMonMtx);

    // stopProactor(acceptTid);
    // pthread_join(monTid, nullptr);
    // pause();
    // close(listenfd);
    // return 0;

}