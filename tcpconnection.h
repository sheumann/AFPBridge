#ifndef TCPCONNECTION_H
#define TCPCONNECTION_H

#include "session.h"

BOOLEAN StartTCPConnection(Session *sess);
void EndTCPConnection(Session *sess);

#endif
