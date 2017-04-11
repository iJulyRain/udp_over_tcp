/*
 * =====================================================================================
 *
 *       Filename:  ezbuf.c
 *
 *    Description:  a simple buffer
 *
 *        Version:  1.0
 *        Created:  2017年04月07日 14时43分30秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  lizhixian (T3), lizhixian@integritytech.com.cn
 *   Organization:  T3
 *
 * =====================================================================================
 */

#include "ezbuf.h"

ezbuf_t *ezbuf_new(void)
{
    ezbuf_t *ezbuf = (ezbuf_t *)malloc(sizeof(ezbuf_t));
    ezbuf_init(ezbuf);

    return ezbuf;
}

void ezbuf_del(ezbuf_t *ezbuf)
{
    free(ezbuf);
}

void ezbuf_init(ezbuf_t *ezbuf)
{
    assert(ezbuf);

    ezbuf->ri = ezbuf->wi = 0;
    memset(ezbuf->buffer, 0, sizeof(EZBUF_MAX));
}

size_t ezbuf_put(ezbuf_t *ezbuf, const char *ptr, int length)
{
    assert(ezbuf);

    if (length >= EZBUF_MAX) //too long
        return -1;

    if ((EZBUF_MAX - (ezbuf->wi - ezbuf->ri)) < length) //left too short
        return -1;

    if (EZBUF_MAX - ezbuf->wi < length) //left space not enough
    {
        memmove(ezbuf->buffer, ezbuf->buffer + ezbuf->ri, ezbuf->wi - ezbuf->ri);
        ezbuf->wi = ezbuf->wi - ezbuf->ri;
        ezbuf->ri = 0;
    }

    memcpy(ezbuf->buffer + ezbuf->wi, ptr, length);
    ezbuf->wi += length;

    return length;
}

size_t ezbuf_get(ezbuf_t *ezbuf, char *ptr, int length)
{
    assert(ezbuf);

    length = ezbuf->wi - ezbuf->ri;

    memcpy(ptr, ezbuf->buffer + ezbuf->ri, length);
    ezbuf->ri += length;

    return length;
}

size_t ezbuf_size(ezbuf_t *ezbuf)
{
    return ezbuf->wi - ezbuf->ri;
}

#if 0

int main(int argc, const char **argv)
{
    int i;
    char buffer[128];

    ezbuf_t *ezbuf = (ezbuf_t *)malloc(sizeof(ezbuf_t));
    assert(ezbuf);

    ezbuf_init(ezbuf);

    for (i = 0; i < 4096; i++)
    {
        memset(buffer, 0, sizeof(buffer));
        strcpy(buffer, "abcdefghijklmnopqrstuvwxyz1234567890");
        ezbuf_put(ezbuf, buffer, strlen(buffer)); 

        printf("[%d]ri:%d wi:%d ==> %s\n", i, ezbuf->ri, ezbuf->wi, buffer);
        memset(buffer, 0, sizeof(buffer));
        ezbuf_get(ezbuf, buffer, 127); 
    }

    return 0;
}
#endif
