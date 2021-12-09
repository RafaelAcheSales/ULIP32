#pragma once
#include <string.h>
char *strdelimit(char *string, const char *delimiters,
                 char new_delimiter)
{
    char *p;
    char c;
    int i;
    int k;

    if (!string || !delimiters)
        return string;

    p = string;
    while ((c = *p)) {
        for (k = 0; delimiters[k]; k++) {
            if (c == delimiters[k]) {
                *p = new_delimiter;
                break;
            }
        }
        p++;
    }

    return string;
}



char *strstrip(char *string)
{
    int len;
    int i;
    int k;

    if (!string) return NULL;

    len = strlen(string);
    for (i = 0; i < len; i++) {
        if (string[i] != ' ')
            break;
    }
    if (i) {
        if (i < len) {
            memcpy(string, string + i, len - i);
            len -= i;
            string[len] = '\0';
        } else {
            string[0] = '\0';
            return string;
        }
    }
    for (k = len - 1; k >= 0; k--) {
        if (string[k] != ' ')
            break;
        string[k] = '\0';
    }

    return string;
}


int strv_length(char **string)
{
    int i = 0;

    if (!string) return 0;

    while (string[i]) i++;

    return i;
}


void strfreev(char **string)
{
    int i = 0;

    if (!string) return;

    while (string[i]) {
        free(string[i]);
        i++;
    }
    free(string);
}

char *strcasestr(const char *haystack,
                 const char *needle)
{
    int hlen;
    int nlen;
    char h;
    char n;
    int rc;
    int k;
    int i;

    if (!haystack || !needle) return NULL;

    hlen = strlen(haystack);
    nlen = strlen(needle);
    if (nlen > hlen) return NULL;
    hlen -= nlen;
    for (i = 0; i <= hlen; i++) {
        rc = 0;
        for (k = 0; k < nlen; k++) {
            h = tolower(haystack[i + k]);
            n = tolower(needle[k]);
            if (h != n) {
                rc = 1;
                break;
            }
        }
        if (!rc) {
            return (char *)&haystack[i + k];
        }
    }

    return NULL;
}
