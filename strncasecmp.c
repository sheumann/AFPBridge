# ifdef __ORCAC__
#  pragma noroot
# endif

#include <stddef.h>
#include <ctype.h>

int strncasecmp(const char *s1, const char *s2, size_t n)
{
    if (n == 0)
        return 0;

    while (n > 1 && *s1 != 0 && tolower(*s1) == tolower(*s2)) {
        s1++;
        s2++;
        n--;
    }
    
    return (int)*s1 - (int)*s2;
}
