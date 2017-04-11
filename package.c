/*
 * =====================================================================================
 *
 *       Filename:  package.c
 *
 *    Description:  package
 *
 *        Version:  1.0
 *        Created:  2017年04月11日 16时27分46秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  lizhixian (T3), lizhixian@integritytech.com.cn
 *   Organization:  T3
 *
 * =====================================================================================
 */

#include "udp_over_tcp.h"

int packet(const char *src, int size, char *dst)
{
    package_t package;

    assert(src);
    assert(dst);

    if (size > PACKAGE_MAX)
    {
        vlog("package too long\n");
        return -1;
    }

    memset(&package, 0, sizeof(package_t));
    package.size = size;
    memcpy(package.data, src, size);

    memcpy(dst, (void *)&package, sizeof(package_t)); //no good

    return sizeof(package_t);
}

int unpack(const char *src, int size, char *dst)
{
    assert(src);
    assert(dst);

    package_t package;
    memcpy((void *)&package, src, size);

    memcpy(dst, package.data, package.size);

    return package.size;
}
