#ifndef AFPURLPARSER_H
#define AFPURLPARSER_H

enum protocol { proto_unknown, proto_AT, proto_TCP, proto_invalid };

typedef struct AFPURLParts {
    enum protocol protocol;
    char *server;
    char *zone;         /* for AppleTalk only */
    char *username;
    char *password;
    char *auth;
    char *volpass;
    char *volume;
} AFPURLParts;

/* Maximum lengths of components (for use in our mounter, using EasyMount) */
#define SERVER_MAX   32
#define ZONE_MAX     32
#define USERNAME_MAX 31
#define PASSWORD_MAX 8
#define VOLPASS_MAX  8
#define VOLUME_MAX   27

#define protoInvalidError           4000
#define noServerOrVolumeNameError   4001
#define serverNameTooLongError      4002
#define volumeNameTooLongError      4003
#define zoneTooLongError            4004
#define usernameTooLongError        4005
#define passwordTooLongError        4006
#define volpassTooLongError         4007
#define passwordWithoutUserError    4008
#define badUAMError                 4009

AFPURLParts parseAFPURL(char *url);
int validateAFPURL(AFPURLParts *urlParts);

#endif
