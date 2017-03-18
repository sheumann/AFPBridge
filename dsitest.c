#include <AppleTalk.h>
#include <locator.h>
#include <tcpip.h>
#include <stdio.h>
#include <orca.h>
#include "aspinterface.h"
#include "atipmapping.h"

ASPGetStatusRec commandRec;
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
    commandRec.async = AT_SYNC;
    commandRec.command = aspGetStatusCommand;
    commandRec.completionPtr = 0;
    commandRec.slsNet = atipMapping.networkNumber;
    commandRec.slsNode = atipMapping.node;
    commandRec.slsSocket = atipMapping.socket;
    commandRec.bufferLength = sizeof(replyBuffer);
    commandRec.bufferAddr = (LongWord)&replyBuffer;
    
    DispatchASPCommand((SPCommandRec *)&commandRec);

    for (i=0; i<commandRec.dataLength;i++) {
        printf("%02x ", replyBuffer[i]);
        if ((i+1)%16 == 0) printf("\n");
    }
    printf("\n");
    for (i=0; i<commandRec.dataLength;i++) {
        if (replyBuffer[i] >= ' ' && replyBuffer[i] <= 126)
            printf("%c", replyBuffer[i]);
        else
            printf(" ");
    }
    printf("\n");
                                     

error:
    if (startedTCP)
        TCPIPShutDown();
    if (loadedTCP)
        UnloadOneTool(54);
    TLShutDown();

}
