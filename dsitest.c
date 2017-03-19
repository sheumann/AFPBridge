#include <AppleTalk.h>
#include <locator.h>
#include <tcpip.h>
#include <stdio.h>
#include <orca.h>
#include "aspinterface.h"
#include "atipmapping.h"

ASPGetStatusRec getStatusRec;
ASPOpenSessionRec openSessionRec;
ASPCloseSessionRec closeSessionRec;
Byte replyBuffer[1024];

int main(int argc, char **argv)
{
    Boolean loadedTCP = FALSE;
    Boolean startedTCP = FALSE;
    cvtRec myCvtRec;
    int i;

    TLStartUp();
    if (!TCPIPStatus()) {
        LoadOneTool(54, 0x0300);    /* load Marinetti 3.0+ */
        if (toolerror())
            goto error;
        loadedTCP = TRUE;
        TCPIPStartUp();
        if (toolerror())
            goto error;
        startedTCP = TRUE;
    }
    
    atipMapping.networkNumber = 0xFFFF;
    atipMapping.node = 0xFF;
    atipMapping.socket = 0xFF;

    TCPIPConvertIPCToHex(&myCvtRec, argv[1]);
    atipMapping.ipAddr = myCvtRec.cvtIPAddress;
    atipMapping.port = 548;
    
    // Do the call
    getStatusRec.async = AT_SYNC;
    getStatusRec.command = aspGetStatusCommand;
    getStatusRec.completionPtr = 0;
    getStatusRec.slsNet = atipMapping.networkNumber;
    getStatusRec.slsNode = atipMapping.node;
    getStatusRec.slsSocket = atipMapping.socket;
    getStatusRec.bufferLength = sizeof(replyBuffer);
    getStatusRec.bufferAddr = (LongWord)&replyBuffer;
    
    DispatchASPCommand((SPCommandRec *)&getStatusRec);

#if 0
    for (i=0; i<getStatusRec.dataLength;i++) {
        printf("%02x ", replyBuffer[i]);
        if ((i+1)%16 == 0) printf("\n");
    }
    printf("\n");
#endif
    for (i=0; i<getStatusRec.dataLength;i++) {
        if (replyBuffer[i] >= ' ' && replyBuffer[i] <= 126)
            printf("%c", replyBuffer[i]);
        else
            printf(" ");
    }
    printf("\n");

    openSessionRec.async = AT_SYNC;
    openSessionRec.command = aspOpenSessionCommand;
    openSessionRec.completionPtr = 0;
    openSessionRec.slsNet = atipMapping.networkNumber;
    openSessionRec.slsNode = atipMapping.node;
    openSessionRec.slsSocket = atipMapping.socket;
    openSessionRec.attnRtnAddr = NULL;  // not used for now

    printf("Opening...\n");
    DispatchASPCommand((SPCommandRec *)&openSessionRec);
    
    printf("refnum = %i\n", openSessionRec.refNum);
    printf("result code = %04x\n", openSessionRec.result);
    if (openSessionRec.result)
        goto error;

    closeSessionRec.async = AT_SYNC;
    closeSessionRec.command = aspCloseSessionCommand;
    closeSessionRec.completionPtr = 0;
    closeSessionRec.refNum = openSessionRec.refNum;

    printf("Closing...\n");
    DispatchASPCommand((SPCommandRec *)&closeSessionRec); 
    printf("result code = %04x\n", closeSessionRec.result);

error:
    if (startedTCP)
        TCPIPShutDown();
    if (loadedTCP)
        UnloadOneTool(54);
    TLShutDown();

}
