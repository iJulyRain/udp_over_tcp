/*
 * =====================================================================================
 *
 *       Filename:  ezbuf.h
 *
 *    Description:  a simple buffer
 *
 *        Version:  1.0
 *        Created:  2017年04月07日 14时44分30秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  lizhixian (T3), lizhixian@integritytech.com.cn
 *   Organization:  T3
 *
 * =====================================================================================
 */

#ifndef __EZBUF_H__
#define __EZBUF_H__

#define EZBUF_MAX   4096 

typedef struct ezbuf
{
    int ri, wi;
    char buffer[EZBUF_MAX];
}ezbuf_t;

void ezbuf_init(ezbuf_t *ezbuf);

size_t ezbuf_put(ezbuf_t *ezbuf, const char *ptr, int length);
size_t ezbuf_get(ezbuf_t *ezbuf, char *ptr, int length);

#endif
