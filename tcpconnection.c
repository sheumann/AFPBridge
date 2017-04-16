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
 * On success, returns TRUE and sets sess->ipid.
 * On failure, returns FALSE and sets commandRec->result to an error code.
 */
BOOLEAN StartTCPConnection(Session *sess) {
    Word tcperr;
    ASPOpenSessionRec *commandRec;
    srBuff mySRBuff;
    LongWord initialTime;
    
    commandRec = (ASPOpenSessionRec *)sess->spCommandRec;
    
    sess->atipMapping = atipMapping;
    
    if (TCPIPGetConnectStatus() == FALSE) {
        TCPIPConnect(NULL);
        if (toolerror()) {
            commandRec->result = aspNetworkErr;
            return FALSE;
        }
    }
    
    sess->ipid = 
        TCPIPLogin(userid(), atipMapping.ipAddr, atipMapping.port, 0, 0x40);
    if (toolerror()) {
        commandRec->result = aspNetworkErr;
        return FALSE;
    }
    
    tcperr = TCPIPOpenTCP(sess->ipid);
    if (toolerror()) {
        TCPIPLogout(sess->ipid);
        commandRec->result = aspNetworkErr;
        return FALSE;
    } else if (tcperr != tcperrOK) {
        TCPIPLogout(sess->ipid);
        commandRec->result = aspNoRespErr;
        return FALSE;
    }
    
    initialTime = GetTick();
    do {
        TCPIPPoll();
        TCPIPStatusTCP(sess->ipid, &mySRBuff);
    } while (mySRBuff.srState == TCPSSYNSENT && GetTick()-initialTime < 15*60);
    if (mySRBuff.srState != TCPSESTABLISHED) {
        TCPIPAbortTCP(sess->ipid);
        TCPIPLogout(sess->ipid);
        commandRec->result = aspNoRespErr;
        return FALSE;
    }
    
    sess->tcpLoggedIn = TRUE;
    return TRUE;
}

void EndTCPConnection(Session *sess) {
    if (sess->tcpLoggedIn) {
        TCPIPCloseTCP(sess->ipid);
        TCPIPPoll();
        TCPIPAbortTCP(sess->ipid);
        TCPIPLogout(sess->ipid);
        sess->tcpLoggedIn = FALSE;
    }
}
