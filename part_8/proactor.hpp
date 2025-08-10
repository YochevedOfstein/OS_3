#pragma once
#include <pthread.h>

typedef void* (*proactorFunc) (int sockfd);

pthread_t startProactor(int sockfd, proactorFunc threadfunc);

int stopProactor(pthread_t tid);