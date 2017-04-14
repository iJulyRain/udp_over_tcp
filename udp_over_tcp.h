/*
 * =====================================================================================
 *
 *       Filename:  udp_over_tcp.h
 *
 *    Description:  udp_over_tcp header
 *
 *        Version:  1.0
 *        Created:  2017年04月07日 20时28分25秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  lizhixian (T3), lizhixian@integritytech.com.cn
 *   Organization:  T3
 *
 * =====================================================================================
 */
#ifndef __IPROXY_H__
#define __IPROXY_H__

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>

#include <assert.h>

#include <time.h>
#include <libgen.h>
#include <getopt.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>

#include <arpa/inet.h>

#include <ev.h>

enum{
    TYPE_UDP_TO_TCP = 0,
    TYPE_TCP_TO_UDP
};

#define obj_entry(node, type, member) \
    ((type *)((char *)(node) - (unsigned long)(&((type *)0)->member)))

#define IP_ADDR_MAX 32

typedef struct config_info{
    char bind_addr[IP_ADDR_MAX];
    char serv_addr[IP_ADDR_MAX];
    int  bind_port;
    int  serv_port;
}cfg, *cfg_p;

#define IO_TIMEOUT_MAX  10

//log
void vlog(const char *format, ...);

//package
#define PACKAGE_MAX 1500 

typedef struct package{
    size_t  size;
    char data[PACKAGE_MAX];
}package_t;

int packet(const char *src, int size, char *dst);
int unpack(const char *src, int size, char *dst);

//relay
void udp_to_tcp_proxy(cfg_p cfg);
void tcp_to_udp_proxy(cfg_p cfg);

#endif
