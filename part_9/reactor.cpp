#include "reactor.hpp"
#include <algorithm>
#include <cmath>
#include <vector>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/select.h>
#include <thread>
#include <atomic>
#include <iostream>
#include <sys/socket.h>
#include <pthread.h>
#include <errno.h> 


struct ProactorData {
    int sockfd;
    proactorFunc func;
};

static void* clientEntry(void* arg) {
    ProactorData* data = static_cast<ProactorData*>(arg);
    int sockfd = data->sockfd;
    proactorFunc func = data->func;
    delete data;                         // matches new
    (void)func(sockfd);                  // run user handler (may close fd)
    return nullptr;
}

static void* acceptEntry(void* arg) {
    // Make this thread cancellable; accept() is a cancellation point
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, nullptr);
    pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, nullptr);

    ProactorData* data = static_cast<ProactorData*>(arg);
    int listenSockfd   = data->sockfd;
    proactorFunc func  = data->func;
    delete data;                         // matches new

    for (;;) {
        int clientSockfd = accept(listenSockfd, nullptr, nullptr);
        if (clientSockfd < 0) {
            if (errno == EINTR) continue;    // interrupted by signal; try again
            perror("accept");
            break;                            // fatal error / listener closed
        }

        // Spawn a detached worker thread
        ProactorData* clientData = new ProactorData{clientSockfd, func};
        pthread_t clientTid;
        if (pthread_create(&clientTid, nullptr, clientEntry, clientData) == 0) {
            pthread_detach(clientTid);
        } else {
            close(clientSockfd);
            delete clientData;
        }
    }
    return nullptr;
}

pthread_t startProactor(int sockfd, proactorFunc threadfunc) {
    pthread_t tid{};
    ProactorData* data = new ProactorData{sockfd, threadfunc};
    if (pthread_create(&tid, nullptr, acceptEntry, data) != 0) {
        std::cerr << "Error creating proactor thread\n";
        delete data;                      // matches new
        return pthread_t{};               // 0 = failure
    }
    return tid;
}

int stopProactor(pthread_t tid) {
    if (!tid) return -1;
    if (pthread_cancel(tid) != 0) return -1;   // stop accept loop
    if (pthread_join(tid, nullptr) != 0) return -1; // reclaim resources
    return 0;
}



// internal struct 
struct reactor {
    struct Watch { int fd; reactorFunc cb; };
    std::vector<Watch> watches;
    int wake_pipe[2];
    std::thread loopThread;
    std::atomic<bool> running;
};

// the background loop: builds fd_set, calls select(), dispatches
static void reactorLoop(reactor* R) {
    while (R->running.load()) {
        fd_set rfds;
        FD_ZERO(&rfds);
        int maxfd = -1;
        for (auto &w : R->watches) {
            FD_SET(w.fd, &rfds);
            if (w.fd > maxfd) maxfd = w.fd;
        }
        if (select(maxfd + 1, &rfds, nullptr, nullptr, nullptr) < 0)
            continue;
        for (auto &w : R->watches) {
            if (FD_ISSET(w.fd, &rfds) && w.cb) {
                w.cb(w.fd);
            }
        }
    }
}

void* startReactor() {

    reactor* R = nullptr;
    try {
        R = new reactor();
    } catch (...) {
        // catches all exceptions including bad_alloc
        return nullptr;
    }

    R->watches = std::vector<reactor::Watch>();
    R->wake_pipe[0] = R->wake_pipe[1] = 0;
    R->loopThread = std::thread();
    R->running = true;

    // create wake-up pipe
    if (pipe(R->wake_pipe) < 0) {
        delete R;
        return nullptr;
    }

    R->watches.push_back({R->wake_pipe[0], nullptr});
    R->loopThread = std::thread(reactorLoop, R);
    return R;
}

int addFdToReactor(void* reactorPtr, int fd, reactorFunc func) {
    if (!reactorPtr || fd < 0) return -1;
    reactor* R = static_cast<reactor*>(reactorPtr);
    R->watches.push_back({fd, func});
    return 0;
}

int removeFdFromReactor(void* reactorPtr, int fd) {
    if (!reactorPtr) return -1;
    auto* R = static_cast<reactor*>(reactorPtr);
    auto& v = R->watches;
    auto it = std::remove_if(v.begin(), v.end(),
        [fd](const reactor::Watch& w){ return w.fd == fd; });
    if (it == v.end()) return -1;
    v.erase(it, v.end());
    return 0;
}

int stopReactor(void* reactorPtr) {
    if (!reactorPtr) return -1;
    reactor* R = static_cast<reactor*>(reactorPtr);
    // signal stop
    R->running = false;
    // wake select()
    write(R->wake_pipe[1], "x", 1);
    // join thread
    if (R->loopThread.joinable())
        R->loopThread.join();
    // close pipes
    close(R->wake_pipe[0]);
    close(R->wake_pipe[1]);
    // cleanup
    delete R;
    return 0;
}
