#include <iostream>
#include <string>
#include <sstream>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>

static constexpr int PORT = 9034;
static constexpr const char* HOST = "127.0.0.1";

bool sendLine(int sock, const std::string& line) {
    std::string out = line + "\n";
    const char* data = out.data();
    size_t toSend = out.size();
    while (toSend) {
        ssize_t n = send(sock, data, toSend, 0);
        if (n < 0) { perror("send"); return false; }
        data    += n;
        toSend  -= n;
    }
    return true;
}

bool recvAndPrint(int sock) {
    char buf[4096];
    ssize_t n = recv(sock, buf, sizeof(buf)-1, 0);
    if (n < 0) { perror("recv"); return false; }
    if (n == 0) { std::cout<<"<server closed>\n"; return false; }
    buf[n]=0;
    std::cout<<buf;
    return true;
}

int main(){
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0){ perror("socket"); return 1; }

    sockaddr_in srv{};
    srv.sin_family = AF_INET;
    srv.sin_port   = htons(PORT);
    inet_pton(AF_INET, HOST, &srv.sin_addr);
    if (connect(sock,(sockaddr*)&srv,sizeof(srv))<0){
        perror("connect"); return 1;
    }

    std::cout<<"Connected. Type commands.\n";
    std::string line;
    while (std::getline(std::cin,line)){
        if (line.empty()) continue;
        // send header
        if (!sendLine(sock,line)) break;

        // if NewGraph, read & send the next n point‐lines
        std::istringstream iss(line);
        std::string cmd; iss>>cmd;
        if (cmd=="NewGraph"){
            int n; iss>>n;
            for(int i=0;i<n;++i){
                std::getline(std::cin,line);
                sendLine(sock,line);
            }
        }

        // now read the one reply and print it
        if (!recvAndPrint(sock)) break;
    }

    close(sock);
    return 0;
}