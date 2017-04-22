#pragma noroot

#include <tcpip.h>
#include <stdlib.h>
#include <AppleTalk.h>
#include <orca.h>
#include <misctool.h>
#include "atipmapping.h"
#include "session.h"

/* Make a TCP connection to the address mapped to the specified AT address.
 * sess->spCommandRec should be an SPGetStatus or SPOpenSession command.
 *
 * On success, returns 0 and sets sess->ipid.
 * On failure, returns an ASP error code.
 */
Word StartTCPConnection(Session *sess) {
    Word tcperr;
    ASPOpenSessionRec *commandRec;
    srBuff mySRBuff;
    LongWord initialTime;
    
    commandRec = (ASPOpenSessionRec *)sess->spCommandRec;
    
    sess->atipMapping = atipMapping;
    
    if (TCPIPGetConnectStatus() == FALSE) {
        TCPIPConnect(NULL);
        if (toolerror())
            return aspNetworkErr;
    }
    
    sess->ipid = 
        TCPIPLogin(userid(), atipMapping.ipAddr, atipMapping.port, 0, 0x40);
    if (toolerror())
        return aspNetworkErr;
    
    tcperr = TCPIPOpenTCP(sess->ipid);
    if (toolerror()) {
        TCPIPLogout(sess->ipid);
        return aspNetworkErr;
    } else if (tcperr != tcperrOK) {
        TCPIPLogout(sess->ipid);
        return aspNoRespErr;
    }
    
    initialTime = GetTick();
    do {
        TCPIPPoll();
        TCPIPStatusTCP(sess->ipid, &mySRBuff);
    } while (mySRBuff.srState == TCPSSYNSENT && GetTick()-initialTime < 15*60);
    if (mySRBuff.srState != TCPSESTABLISHED) {
        TCPIPAbortTCP(sess->ipid);
        TCPIPLogout(sess->ipid);
        return aspNoRespErr;
    }
    
    sess->tcpLoggedIn = TRUE;
    return 0;
}

void EndTCPConnection(Session *sess) {
    if (sess->tcpLoggedIn) {
        TCPIPPoll();
        TCPIPAbortTCP(sess->ipid);
        TCPIPLogout(sess->ipid);
        sess->tcpLoggedIn = FALSE;
    }
}
