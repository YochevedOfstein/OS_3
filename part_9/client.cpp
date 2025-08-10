#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>

static constexpr int PORT = 9034;
static constexpr const char* HOST = "127.0.0.1";

// Send a line (with trailing '\n') to the socket
bool sendLine(int sock, const std::string& line) {
    std::string out = line + "\n";
    const char* data = out.data();
    size_t toSend = out.size();
    while (toSend > 0) {
        ssize_t sent = send(sock, data, toSend, 0);
        if (sent < 0) {
            perror("send");
            return false;
        }
        data    += sent;
        toSend  -= sent;
    }
    return true;
}

// Receive up to bufSize-1 bytes, null-terminate, and print
bool recvAndPrint(int sock) {
    char buf[4096];
    ssize_t recvd = recv(sock, buf, sizeof(buf)-1, 0);
    if (recvd < 0) {
        perror("recv");
        return false;
    }
    if (recvd == 0) {
        std::cout << "Server closed connection\n";
        return false;
    }
    buf[recvd] = '\0';
    std::cout << buf;
    return true;
}

int main() {
    // 1) connect
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) { perror("socket"); return 1; }

    sockaddr_in serv{};
    serv.sin_family = AF_INET;
    serv.sin_port   = htons(PORT);
    if (inet_pton(AF_INET, HOST, &serv.sin_addr) <= 0) {
        std::cerr << "Invalid address\n";
        return 1;
    }
    if (connect(sock, (sockaddr*)&serv, sizeof(serv)) < 0) {
        perror("connect");
        return 1;
    }

    std::cout << "Connected to " << HOST << ":" << PORT 
              << "\nType commands, Ctrl-D to exit.\n";

    std::string line;
    while (std::getline(std::cin, line)) {
        if (line.empty()) continue;

        // Parse the command and possible n
        std::istringstream iss(line);
        std::string cmd;
        iss >> cmd;

        // Send the header line
        if (!sendLine(sock, line)) break;

        // If it's NewGraph, read & send the next n lines in a row
        if (cmd == "NewGraph") {
            int n;
            if (!(iss >> n)) {
                std::cerr << "Syntax: NewGraph <n>\n";
                continue;
            }
            for (int i = 0; i < n; ++i) {
                if (!std::getline(std::cin, line)) {
                    std::cerr << "Unexpected EOF reading points\n";
                    break;
                }
                if (line.empty()) { --i; continue; }
                if (!sendLine(sock, line)) break;
            }
        }

        // Now wait for and print the server's reply
        if (!recvAndPrint(sock)) break;
    }

    close(sock);
    std::cout << "Client exiting\n";
    return 0;
}