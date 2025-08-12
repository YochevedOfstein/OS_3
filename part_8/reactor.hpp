#pragma once
#include <pthread.h>


typedef void* (*reactorFunc)(int fd);

typedef struct reactor reactor;

void *startReactor ();

int addFdToReactor(void *reactor, int fd, reactorFunc func);

int removeFdFromReactor(void *reactor, int fd);

int stopReactor(void *reactor);

typedef void* (*proactorFunc) (int sockfd);

pthread_t startProactor(int sockfd, proactorFunc threadfunc);

int stopProactor(pthread_t tid);