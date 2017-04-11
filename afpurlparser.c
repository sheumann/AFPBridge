#include <string.h>
#include <ctype.h>
#include "afpurlparser.h"


#ifdef AFPURLPARSER_TEST
# include <stdio.h>
#else
# ifdef __ORCAC__
#  pragma noroot
# endif
#endif

int strncasecmp(const char *s1, const char *s2, size_t n)
{
    while (tolower(*s1) == tolower(*s2) && *s1 != 0 && n > 1) {
        s1++;
        s2++;
        n--;
    }
    
    return (int)*s1 - (int)*s2;
}

static int hextonum(char c) {
    if (c >= '0' && c <= '9')
        return c - '0';
    if (c >= 'a' && c <= 'f')
        return c - 'a' + 10;
    if (c >= 'A' && c <= 'F')
        return c - 'A' + 10;
    return 0;
}

static void parseEscapes(char *url)
{
    unsigned int i, j;
    unsigned int len;
    
    len = strlen(url);
    
    for (i = 0, j = 0; url[i] != 0; j++) {
        if (url[i] == '%' && isxdigit(url[i+1]) && isxdigit(url[i+2]))
        {
            url[j] = hextonum(url[i+1]) * 16 + hextonum(url[i+2]);
            i += 3;
        } else {
            url[j] = url[i];
            i++;
        }
    }
    url[j] = 0;
}

AFPURLParts parseAFPURL(char *url)
{
    char *sep;
    AFPURLParts urlParts = {0};

    parseEscapes(url);
    
    urlParts.protocol = proto_unknown;
    
    if (strncasecmp(url, "afp:", 4) == 0)
        url = url + 4;

    if (strncmp(url, "//", 2) == 0) {
        urlParts.protocol = proto_TCP;
        url += 2;
    } else if (strncasecmp(url, "/at/", 4) == 0) {
        urlParts.protocol = proto_AT;
        url += 4;
    }
    
    /* Discard extra slashes */
    while (*url == '/')
        url++;
    
    sep = strchr(url, '@');
    if (sep) {
        *sep = 0;
        urlParts.username = url;
        urlParts.server = sep + 1;
    } else {
        urlParts.username = NULL;
        urlParts.server = url;
    }
    
    if (urlParts.username != NULL) {
        sep = strchr(urlParts.username, ':');
        
        if (sep) {
            *sep = 0;
            urlParts.password = sep + 1;
        }
        
        sep = strrchr(urlParts.username, ';');        
        if (sep && strncmp(sep, ";VOLPASS=", 9) == 0) {
            *sep = 0;
            urlParts.volpass = sep + 9;
        }
        
        sep = strrchr(urlParts.username, ';');
        if (sep && strncmp(sep, ";AUTH=", 6) == 0) {
            *sep = 0;
            urlParts.auth = sep + 6;
        }
    }
    
    sep = strchr(urlParts.server, '/');
    if (sep) {
        *sep = 0;
        urlParts.volume = sep + 1;
        
        /* strip trailing slashes */
        while ((sep = strrchr(urlParts.volume, '/')) != NULL && sep[1] == 0)
            *sep = 0;
    }
    
    sep = strchr(urlParts.server, ':');
    if (sep && (urlParts.protocol == proto_AT || 
        (urlParts.protocol == proto_unknown 
            && strspn(sep+1, "0123456789") != strlen(sep+1))))
    {
        *sep = 0;
        urlParts.zone = sep + 1;
        urlParts.protocol = proto_AT;
    }
    
    if (urlParts.server == NULL)
        urlParts.protocol = proto_invalid;
    
    return urlParts;
}

/* Check if an AFP URL is suitable for our AFP mounter */
int validateAFPURL(AFPURLParts *urlParts)
{
    if (urlParts->server == NULL || urlParts->volume == NULL)
        return noServerOrVolumeNameError;
    if (urlParts->protocol == proto_invalid)
        return protoInvalidError;
    if (strlen(urlParts->server) > SERVER_MAX)
        return serverNameTooLongError;
    if (strlen(urlParts->volume) > VOLUME_MAX)
        return volumeNameTooLongError;
    if (urlParts->zone != NULL && strlen(urlParts->zone) > ZONE_MAX)
        return zoneTooLongError;
    if (urlParts->username != NULL && strlen(urlParts->username) > USERNAME_MAX)
        return usernameTooLongError;
    if (urlParts->password != NULL && strlen(urlParts->password) > PASSWORD_MAX)
        return passwordTooLongError;
    if (urlParts->volpass != NULL && strlen(urlParts->volpass) > VOLPASS_MAX)
        return volpassTooLongError;
    
    /* Must have username & password, or neither */
    if (urlParts->username == NULL) 
        if (urlParts->password != NULL && *urlParts->password != 0)
            return userXorPasswordError;
    if (urlParts->password == NULL) 
        if (urlParts->username != NULL && *urlParts->username != 0)
            return userXorPasswordError;

    if (urlParts->auth != NULL) {
        if (strncasecmp(urlParts->auth, "No User Authent", 16) == 0) {
            if ((urlParts->username != NULL && *urlParts->username != 0)
                || (urlParts->password != NULL && *urlParts->password != 0))
                return badUAMError;
        } else if (strncasecmp(urlParts->auth, "Randnum Exchange", 17) == 0) {
            if (urlParts->username == NULL || *urlParts->username == 0
                || urlParts->password == NULL)
                return badUAMError;
        } else {
            /* unknown/unsupported UAM */
            return badUAMError;
        }
    }
    
    return 0;
}

#ifdef AFPURLPARSER_TEST
int main(int argc, char **argv)
{
    AFPURLParts urlParts;

    if (argc < 2)
        return 1;
    
    urlParts = parseAFPURL(strdup(argv[1]));
    
    printf("protocol = ");
    switch(urlParts.protocol) {
    case proto_unknown: printf("unknown\n");            break;
    case proto_AT:      printf("AppleTalk\n");          break;
    case proto_TCP:     printf("TCP\n");                break;
    case proto_invalid: printf("invalid URL\n");        break;
    default:            printf("Bad URL structure\n");  break;
    }
    
    printf("server:   %s\n", urlParts.server);
    printf("zone:     %s\n", urlParts.zone);
    printf("username: %s\n", urlParts.username);
    printf("password: %s\n", urlParts.password);
    printf("auth:     %s\n", urlParts.auth);
    printf("volpass:  %s\n", urlParts.volpass);
    printf("volume:   %s\n", urlParts.volume);
    
    printf("Validation result: %s\n", validateAFPURL(&urlParts) ? "bad" : "OK");
}
#endif

