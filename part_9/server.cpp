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
#include <thread>
#include <signal.h>

static constexpr int PORT = 9034;

Graph graph;
std::mutex graphMutex;

static volatile sig_atomic_t gStopFlag = 0;
static void on_stop(int) {
    gStopFlag = 1;
}

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


static void* handleClient(int clientSocket) {
    std::string line;

    while (recvLine(clientSocket, line)) {
        if (line.empty()) continue;
        std::istringstream in(line);
        std::string cmd;
        in >> cmd;

        if (cmd == "Newgraph") {
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
            double area;
            {
                std::lock_guard<std::mutex> lock(graphMutex);
                area = graph.area();
            }
            std::ostringstream out;
            out << "Area = " << area << std::endl;
            sendAll(clientSocket, out.str());

        } else if (cmd == "Newpoint") {
            double x, y; char comma;
            in >> x >> comma >> y;
            if (comma != ',') {
                sendAll(clientSocket, "Invalid point format\n");
                continue;
            }

            {
                std::lock_guard<std::mutex> lock(graphMutex);
                if (!graph.addPoint(Point{x, y})) {
                    sendAll(clientSocket, "Failed to add point (duplicate)\n");
                    continue;
                }
            }
            std::ostringstream response;
            response << "Point added: " << x << "," << y << "\n";
            sendAll(clientSocket, response.str());

        } else if (cmd == "Removepoint") {
            double x, y; char comma;
            in >> x >> comma >> y;
            if (comma != ',') {
                sendAll(clientSocket, "Invalid point format\n");
                continue;
            }

            {
                std::lock_guard<std::mutex> lock(graphMutex);
                if (!graph.removePoint(Point{x, y})) {
                    sendAll(clientSocket, "Failed to remove point (not found)\n");
                    continue;
                }
            }
            std::ostringstream response;
            response << "Point removed: " << x << "," << y << "\n";
            sendAll(clientSocket, response.str());
            

        } else if (cmd == "Addedge") {
            double x1, y1, x2, y2; char comma1, comma2;
            in >> x1 >> comma1 >> y1 >> x2 >> comma2 >> y2;
            {
                std::lock_guard<std::mutex> lock(graphMutex);
                if (!graph.addEdge(Point{x1, y1}, Point{x2, y2})) {
                    sendAll(clientSocket, "Failed to add edge (duplicate)\n");
                    continue;
                }
            }
            std::ostringstream response;
            response << "Edge added: (" << x1 << "," << y1 << ") - (" << x2 << "," << y2 << ")\n";
            sendAll(clientSocket, response.str());

        } else if (cmd == "Removeedge") {
            double x1, y1, x2, y2; char comma1, comma2;
            in >> x1 >> comma1 >> y1 >> x2 >> comma2 >> y2;
            {
                std::lock_guard<std::mutex> lock(graphMutex);
                if (!graph.removeEdge(Point{x1, y1}, Point{x2, y2})) {
                    sendAll(clientSocket, "Failed to remove edge (not found)\n");
                    continue;
                }
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
    signal(SIGPIPE, SIG_IGN);
    struct sigaction sa;
    sa.sa_handler = on_stop;
    sigaction(SIGINT, &sa, nullptr);
    sigaction(SIGTERM, &sa, nullptr);

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

    int yes = 1;
    setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));

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

    pthread_t acceptTid = startProactor(listenfd, &handleClient);
    if (!acceptTid) {
        std::cerr << "Error starting proactor thread\n";
        close(listenfd);
        return 1;
    }

    pause();
    std::cout << "Shutting down proactor...\n";
    stopProactor(acceptTid);
    std::cout << "Proactor stopped\n";
    close(listenfd);
    return 0;

}