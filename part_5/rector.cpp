// #include "rector.hpp"
// #include <algorithm>
// #include <cmath>
// #include <vector>
// #include <stdlib.h>
// #include <string.h>
// #include <unistd.h>
// #include <sys/select.h>
// #include <thread>
// #include <atomic>



// // internal struct matching the opaque handle
// struct reactor {
//     struct Watch { int fd; reactorFunc cb; };
//     std::vector<Watch> watches;
//     int wake_pipe[2];
//     std::thread loopThread;
//     std::atomic<bool> running;
// };

// // the background loop: builds fd_set, calls select(), dispatches
// static void reactorLoop(reactor* R) {
//     while (R->running.load()) {
//         fd_set rfds;
//         FD_ZERO(&rfds);
//         int maxfd = -1;
//         for (auto &w : R->watches) {
//             FD_SET(w.fd, &rfds);
//             if (w.fd > maxfd) maxfd = w.fd;
//         }
//         if (select(maxfd + 1, &rfds, nullptr, nullptr, nullptr) < 0)
//             continue;
//         for (auto &w : R->watches) {
//             if (FD_ISSET(w.fd, &rfds) && w.cb) {
//                 w.cb(w.fd);
//             }
//         }
//     }
// }

// void* startReactor() {
//     reactor* R = nullptr;
//     try {
//         R = new reactor{std::vector<reactor::Watch>{}, {0,0}, {}, true};
//     } catch (...) {
//         return nullptr;
//     }
//     // create wake-up pipe
//     if (pipe(R->wake_pipe) < 0) {
//         delete R;
//         return nullptr;
//     }
//     // always watch the read end so we can wake select()
//     R->watches.push_back({R->wake_pipe[0], nullptr});
//     R->running = true;
//     // launch loop thread
//     R->loopThread = std::thread(reactorLoop, R);
//     return R;
// }

// int addFdToReactor(void* reactorPtr, int fd, reactorFunc func) {
//     if (!reactorPtr || fd < 0) return -1;
//     reactor* R = static_cast<reactor*>(reactorPtr);
//     R->watches.push_back({fd, func});
//     return 0;
// }

// int removeFdFromReactor(void* reactorPtr, int fd) {
//     if (!reactorPtr) return -1;
//     reactor* R = static_cast<reactor*>(reactorPtr);
//     auto it = std::remove_if(R->watches.begin(), R->watches.end(),
//         [&](auto &w){ return w.fd == fd; });
//     if (it == R->watches.end()) return -1;
//     R->watches.erase(it, R->watches.end());
//     return 0;
// }

// int stopReactor(void* reactorPtr) {
//     if (!reactorPtr) return -1;
//     reactor* R = static_cast<reactor*>(reactorPtr);
//     // signal stop
//     R->running = false;
//     // wake select()
//     write(R->wake_pipe[1], "x", 1);
//     // join thread
//     if (R->loopThread.joinable())
//         R->loopThread.join();
//     // close pipes
//     close(R->wake_pipe[0]);
//     close(R->wake_pipe[1]);
//     // cleanup
//     delete R;
//     return 0;
// }