#ifndef READTCP_H
#define READTCP_H

#include "session.h"

typedef enum ReadStatus {
    rsDone,
    rsWaiting,
    rsError
} ReadStatus;

void InitReadTCP(Session *sess, LongWord readCount, void *readPtr);
ReadStatus TryReadTCP(Session *sess);

#endif
