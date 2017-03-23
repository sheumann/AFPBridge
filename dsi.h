#ifndef DSI_H
#define DSI_H

#include "dsiproto.h"
#include "session.h"

void SendDSIMessage(Session *sess, DSIRequestHeader *header, void *payload,
                    void *extraPayload);
void PollForData(Session *sess);

#endif
