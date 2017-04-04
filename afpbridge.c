#include <locator.h>
#include <tcpip.h>
#include <orca.h>
#include <stdlib.h>
#include "installcmds.h"
#include "aspinterface.h"
#include "dsi.h"
#include "asmglue.h"

int main(void) {
    Boolean loadedTCP = FALSE;
    Boolean startedTCP = FALSE;
    
    if (!TCPIPStatus() || toolerror()) {
        LoadOneTool(54, 0x0300);    /* load Marinetti 3.0+ */
        if (toolerror())
            goto error;
        loadedTCP = TRUE;
        TCPIPStartUp();
        if (toolerror())
            goto error;
        startedTCP = TRUE;
    }

    installCmds();
    
    while (TRUE) {
        IncBusyFlag();
        PollAllSessions();
        DecBusyFlag();
        asm { cop 0x7f }
    }
    
error:
    if (startedTCP)
        TCPIPShutDown();
    if (loadedTCP)
        UnloadOneTool(54);
    return;
}
