#include "Graph.hpp"
#include <iostream>
#include <vector>
#include <algorithm>
#include <cmath>
#include <sstream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <map>
#include <signal.h>


static constexpr int PORT = 9034;
static Graph gGraph;

struct ConnState {
    std::string inbuf;            // bytes accumulated until '\n'
    int expect_points = 0;        // >0 means NewGraph is waiting for N point lines
    std::vector<Point> pending;   // temp points for NewGraph
};


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
            return;
        }
        msg += n;
        msgLength -= n;
    }
}


static std::string processLine(ConnState& st, const std::string& rawLine) {
    std::string line = rawLine;
    if (!line.empty() && line.back() == '\r') line.pop_back();

    // If we're in the middle of NewGraph, treat the line as a point.
    if (st.expect_points > 0) {
        double x, y; char comma;
        std::istringstream ptin(line);
        if (!(ptin >> x >> comma >> y) || comma != ',' || (ptin >> std::ws, !ptin.eof())) {
            std::ostringstream err; err << "Invalid point format: " << line << "\n";
            st.expect_points = 0; st.pending.clear();
            return err.str();
        }
        st.pending.push_back(Point{x, y});
        if (--st.expect_points == 0) {
            gGraph.newGraph(st.pending);
            st.pending.clear();
            return "New graph created\n";
        }
        return ""; // Not done yet; wait for more point lines (no reply yet)
    }

    // Otherwise parse a command line
    if (line.empty()) return "";

    std::istringstream in(line);
    std::string cmd; in >> cmd;

    if (cmd == "Newgraph") {
        int n; 
        if (!(in >> n) || n < 0 || (in >> std::ws, !in.eof())) {
            return "Invalid Newgraph count\n";
        }
        st.expect_points = n;
        st.pending.clear();
        if (n == 0) {
            gGraph.newGraph({});
            st.expect_points = 0;
            return "New graph created\n";
        }
        return ""; // wait for the n point lines

    } else if (cmd == "CH") {
        double area = gGraph.area();
        std::ostringstream out;
        out << "Area = " << area << "\n";
        return out.str();

    } else if (cmd == "Newpoint") {
        double x, y; char comma;
        if (!(in >> x >> comma >> y) || comma != ',' || (in >> std::ws, !in.eof())) {
            std::ostringstream err; err << "Invalid point format: " << line << "\n";
            return err.str();
        }
        gGraph.addPoint(Point{x, y});
        return "Point added\n";

    } else if (cmd == "Removepoint") {
        double x, y; char comma;
        if (!(in >> x >> comma >> y) || comma != ',' || (in >> std::ws, !in.eof())) {
            std::ostringstream err; err << "Invalid point format: " << line << "\n";
            return err.str();
        }
        gGraph.removePoint(Point{x, y});
        return "Point removed\n";

    } else if (cmd == "Addedge") {
        double x1, y1, x2, y2; char c1, c2;
        if (!(in >> x1 >> c1 >> y1 >> x2 >> c2 >> y2) || c1 != ',' || c2 != ',' || (in >> std::ws, !in.eof())) {
            std::ostringstream err; err << "Invalid edge format: " << line << "\n";
            return err.str();
        }
        gGraph.addEdge(Point{x1, y1}, Point{x2, y2});
        return "Edge added\n";

    } else if (cmd == "Removeedge") {
        double x1, y1, x2, y2; char c1, c2;
        if (!(in >> x1 >> c1 >> y1 >> x2 >> c2 >> y2) || c1 != ',' || c2 != ',' || (in >> std::ws, !in.eof())) {
            std::ostringstream err; err << "Invalid edge format: " << line << "\n";
            return err.str();
        }
        gGraph.removeEdge(Point{x1, y1}, Point{x2, y2});
        return "Edge removed\n";
    }

    return "Unknown command\n";
}


int main() {
    signal(SIGPIPE, SIG_IGN);
    std::cout << "Starting Graph server on port " << PORT << "...\n";

    int listenfd = socket(AF_INET, SOCK_STREAM, 0);
    if (listenfd < 0) { std::cerr << "Error creating socket\n"; return 1; }

    int yes = 1;
    setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));

    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(PORT);
    if (bind(listenfd, (sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        std::cerr << "Error binding socket\n";
        close(listenfd);
        return 1;
    }

    if (listen(listenfd, SOMAXCONN) < 0) {
        std::cerr << "Error listening on socket\n";
        close(listenfd);
        return 1;
    }

    fd_set master, readfds;
    FD_ZERO(&master);
    FD_SET(listenfd, &master);
    int fdmax = listenfd;

    std::map<int, ConnState> conns;

    for (;;) {
        readfds = master;
        if (select(fdmax + 1, &readfds, nullptr, nullptr, nullptr) < 0) {
            if (errno == EINTR) continue;
            perror("select");
            break;
        }

        for (int fd = 0; fd <= fdmax; ++fd) if (FD_ISSET(fd, &readfds)) {
            if (fd == listenfd) {
                int cfd = accept(listenfd, nullptr, nullptr);
                if (cfd < 0) { perror("accept"); continue; }
                FD_SET(cfd, &master);
                if (cfd > fdmax) fdmax = cfd;
                conns.emplace(cfd, ConnState{});
                std::cout << "Client connected\n";
            } else {
                char buf[4096];
                ssize_t n = recv(fd, buf, sizeof(buf), 0);
                if (n <= 0) {
                    // client closed or error
                    close(fd);
                    FD_CLR(fd, &master);
                    conns.erase(fd);
                    std::cout << "Client disconnected\n";
                    continue;
                }

                ConnState& st = conns[fd];
                st.inbuf.append(buf, n);

                // Process all complete lines
                for (;;) {
                    auto pos = st.inbuf.find('\n');
                    if (pos == std::string::npos) break;

                    std::string line = st.inbuf.substr(0, pos + 1);
                    st.inbuf.erase(0, pos + 1);

                    std::string reply = processLine(st, line);
                    if (!reply.empty()) {
                        const char* p = reply.data(); size_t left = reply.size();
                        while (left) {
                            ssize_t w = send(fd, p, left, 0);
                            if (w <= 0) { // client vanished
                                close(fd); FD_CLR(fd, &master); conns.erase(fd);
                                break;
                            }
                            p += w; left -= w;
                        }
                    }
                }
            }
        }
    }

    close(listenfd);
    return 0;
}