#include "proactor.hpp"
#include <iostream>
#include <unistd.h>
#include <sys/socket.h>
#include <pthread.h>

struct ProactorData {
    int sockfd;
    proactorFunc func;
};

static void* clientEntry(void* arg) {
    ProactorData* data = static_cast<ProactorData*>(arg);
    int sockfd = data->sockfd;
    proactorFunc func = data->func;
    free(data); // Free the allocated memory for ProactorData
    func(sockfd);

    return nullptr;
}

static void* acceptEntry(void* arg) {
    // Set the thread cancellation state and type - will promptly stop the accept loop while it's blocked in accept()
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, nullptr);
    pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, nullptr);

    ProactorData* data = static_cast<ProactorData*>(arg);
    int listenSockfd = data->sockfd;
    proactorFunc func = data->func;
    free(data); // Free the allocated memory for ProactorData

    while (true) {
        // Accept connections and call the provided function
        int clientSockfd = accept(listenSockfd, nullptr, nullptr);
        if (clientSockfd < 0) {
            std::cerr << "Error accepting connection\n";
            continue;
        }
        ProactorData* clientData = (ProactorData*)malloc(sizeof(ProactorData));
        clientData->sockfd = clientSockfd;
        clientData->func = func;

        pthread_t clientTid;
        if (pthread_create(&clientTid, nullptr, clientEntry, clientData) == 0) {
            pthread_detach(clientTid);
        }
        else{
            close(clientSockfd);
            free(clientData);
        }
    }

    return nullptr;
}

pthread_t startProactor(int sockfd, proactorFunc threadfunc) {
    pthread_t tid;
    ProactorData* data = new ProactorData{sockfd, threadfunc};
    if (pthread_create(&tid, nullptr, acceptEntry, data) != 0) {
        std::cerr << "Error creating proactor thread\n";
        free(data);
        return pthread_t{};
    }
    return tid;
}

int stopProactor(pthread_t tid) {
    if(!tid) return -1;
    if (pthread_cancel(tid) != 0) {
        return -1;
    }
    if (pthread_join(tid, nullptr) != 0) {
        return -1;
    }
    return 0;
}
