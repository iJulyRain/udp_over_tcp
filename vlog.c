/*
 * =====================================================================================
 *
 *       Filename:  vlog.c
 *
 *    Description:  vlog
 *
 *        Version:  1.0
 *        Created:  2017年04月08日 15时21分57秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  lizhixian (T3), lizhixian@integritytech.com.cn
 *   Organization:  T3
 *
 * =====================================================================================
 */

#include "udp_over_tcp.h"

int verbose = 0;

void vlog(const char *format, ...)
{
    va_list ap; 
    time_t now;
    char tbuf[32];

    if (verbose == 0)
        return;

    memset(tbuf, 0, sizeof(tbuf));

    now = time(NULL);
    ctime_r(&now, tbuf);
    tbuf[strlen(tbuf) - 1] = '\0';
	printf("[%s]  ", tbuf);

    va_start(ap, format);
    vprintf(format, ap);
    va_end(ap);
}
