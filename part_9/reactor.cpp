#include "reactor.hpp"
#include <algorithm>
#include <new>
#include <thread>
#include <atomic>
#include <vector>
#include <unistd.h>
#include <sys/select.h>
#include <fcntl.h>
#include <errno.h>
#include <iostream>
#include <sys/socket.h>



struct ProactorData {
    int sockfd;
    proactorFunc func;
};

static void* clientEntry(void* arg) {
    ProactorData* data = static_cast<ProactorData*>(arg);
    int sockfd = data->sockfd;
    proactorFunc func = data->func;
    delete data;                      
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
    delete data;     

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



enum CmdType { CMD_ADD, CMD_RM, CMD_STOP };

struct Cmd {
    CmdType     t;
    int         fd;
    reactorFunc cb;
};

// ----- reactor internals -----
struct reactor {
    struct Watch { int fd; reactorFunc cb; };

    std::vector<Watch> watches;   // mutated ONLY in reactor thread
    int wake_pipe[2];             // [0]=read, [1]=write (both nonblocking)
    std::thread loopThread;
    std::atomic<bool> running;

    reactor() : watches(), wake_pipe{-1,-1}, running(false) {}
};

// best-effort, nonblocking
static void sendCmd(reactor* R, const Cmd& c) {
    if (!R) return;
    (void)!write(R->wake_pipe[1], &c, sizeof(c));
}

// read all queued commands and apply them (runs in reactor thread)
static void drainAndApply(reactor* R) {
    for (;;) {
        Cmd c;
        ssize_t n = read(R->wake_pipe[0], &c, sizeof(c));
        if (n != (ssize_t)sizeof(c)) break; // pipe empty (or short read)
        if (c.t == CMD_ADD) {
            // update if exists, else push
            std::vector<reactor::Watch>::iterator it =
                std::find_if(R->watches.begin(), R->watches.end(),
                    [&](const reactor::Watch& w){ return w.fd == c.fd; });
            if (it != R->watches.end()) it->cb = c.cb;
            else R->watches.push_back(reactor::Watch{c.fd, c.cb});
        } else if (c.t == CMD_RM) {
            R->watches.erase(
                std::remove_if(R->watches.begin(), R->watches.end(),
                    [&](const reactor::Watch& w){ return w.fd == c.fd; }),
                R->watches.end());
        } else if (c.t == CMD_STOP) {
            R->running = false;
        }
    }
}

static void reactorLoop(reactor* R) {
    while (R->running.load()) {
        fd_set rfds; FD_ZERO(&rfds);
        int maxfd = -1;

        // always watch the wake pipe
        FD_SET(R->wake_pipe[0], &rfds);
        if (R->wake_pipe[0] > maxfd) maxfd = R->wake_pipe[0];

        // watch all registered fds
        for (size_t i = 0; i < R->watches.size(); ++i) {
            FD_SET(R->watches[i].fd, &rfds);
            if (R->watches[i].fd > maxfd) maxfd = R->watches[i].fd;
        }

        int rc = select(maxfd + 1, &rfds, NULL, NULL, NULL);
        if (rc < 0) {
            if (errno == EINTR) continue;
            break;
        }

        // first apply pending config changes
        if (FD_ISSET(R->wake_pipe[0], &rfds)) {
            drainAndApply(R);
        }
        if (!R->running.load()) break;

        // now dispatch callbacks (changes take effect next loop)
        for (size_t i = 0; i < R->watches.size(); ++i) {
            reactorFunc cb = R->watches[i].cb;
            int fd = R->watches[i].fd;
            if (cb && FD_ISSET(fd, &rfds)) {
                (void)cb(fd); // ignore returned void*
            }
        }
    }
}

void* startReactor() {
    reactor* R = new (std::nothrow) reactor();
    if (!R) return NULL;

    if (pipe(R->wake_pipe) < 0) { delete R; return NULL; }

    // make both ends nonblocking
    for (int i = 0; i < 2; ++i) {
        int flags = fcntl(R->wake_pipe[i], F_GETFL, 0);
        fcntl(R->wake_pipe[i], F_SETFL, flags | O_NONBLOCK);
    }

    R->running = true;
    R->loopThread = std::thread(reactorLoop, R);
    return R;
}

int addFdToReactor(void* rp, int fd, reactorFunc func) {
    if (!rp || fd < 0) return -1;
    sendCmd(static_cast<reactor*>(rp), Cmd{CMD_ADD, fd, func});
    return 0;
}

int removeFdFromReactor(void* rp, int fd) {
    if (!rp) return -1;
    sendCmd(static_cast<reactor*>(rp), Cmd{CMD_RM, fd, NULL});
    return 0;
}

int stopReactor(void* rp) {
    if (!rp) return -1;
    reactor* R = static_cast<reactor*>(rp);

    // tell loop to stop and join it
    sendCmd(R, Cmd{CMD_STOP, -1, NULL});
    if (R->loopThread.joinable()) R->loopThread.join();

    if (R->wake_pipe[0] != -1) close(R->wake_pipe[0]);
    if (R->wake_pipe[1] != -1) close(R->wake_pipe[1]);
    delete R;
    return 0;
}