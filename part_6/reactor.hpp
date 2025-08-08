#pragma once


typedef void (*reactorFunc)(int fd);

typedef struct reactor reactor;

void *startReactor ();

int addFdToReactor(void *reactor, int fd, reactorFunc func);

int removeFdFromReactor(void *reactor, int fd);

int stopReactor(void *reactor);