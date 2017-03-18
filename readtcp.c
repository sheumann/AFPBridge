#pragma noroot

#include "readtcp.h"
#include "session.h"
#include <tcpip.h>
#include <orca.h>

#define buffTypePointer 0x0000      /* For TCPIPReadTCP() */
#define buffTypeHandle 0x0001
#define buffTypeNewHandle 0x0002

void InitReadTCP(Session *sess, LongWord readCount, void *readPtr) {
    sess->readCount = readCount;
    sess->readPtr = readPtr;
}


ReadStatus TryReadTCP(Session *sess) {
    rrBuff rrBuff;

    TCPIPPoll();
    sess->tcperr = TCPIPReadTCP(sess->ipid, buffTypePointer, (Ref)sess->readPtr, 
                                sess->readCount, &rrBuff);
    sess->toolerr = toolerror();
    if (sess->tcperr || sess->toolerr) {
        sess->dsiStatus = error;
        return rsError;
    }
    
    sess->readCount -= rrBuff.rrBuffCount;
    sess->readPtr += rrBuff.rrBuffCount;
    
    if (sess->readCount == 0) {
        return rsDone;
    } else {
        return rsWaiting;
    }
}
