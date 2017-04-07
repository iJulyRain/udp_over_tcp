/*
 * =====================================================================================
 *
 *       Filename:  utpproxy.c
 *
 *    Description:  utpproxy
 *
 *        Version:  1.0
 *        Created:  2017年04月01日 17时14分22秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  lizhixian (T3), lizhixian@integritytech.com.cn
 *   Organization:  T3
 *
 * =====================================================================================
 */

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

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

const char *relay_type[] = {"udp_to_tcp", "tcp_to_udp"};

static int verbose = 0;

#define IP_ADDR_MAX 32

typedef struct config_info{
    char bind_addr[IP_ADDR_MAX];
    char serv_addr[IP_ADDR_MAX];
    int  bind_port;
    int  serv_port;
}cfg, *cfg_p;

void usage(char *name)
{
    printf(
        "usage: \n"
        "\t%s -v -t u2t -b 0.0.0.0 -l 4500 -s 192.168.1.111 -p 4500\n",
        basename(name)
    );
}

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

void udp_to_tcp_proxy(cfg_p cfg)
{
    vlog("udp_to_tcp proxy relay\n");
}

void tcp_to_udp_proxy(cfg_p cfg)
{
    vlog("tcp_to_udp proxy relay\n");
}

int main(int argc, char **argv)
{
    int option;
    int type = 0;

    cfg cfg;
    memset(&cfg, 0, sizeof(cfg));

    while ((option = getopt(argc, argv, "b:l:s:p:t:v")) > 0)
    {
        switch (option)
        {
            case 't':
                if (!strcmp(optarg, "u2p"))
                    type = TYPE_UDP_TO_TCP;
                else if (!strcmp(optarg, "t2u"))
                    type = TYPE_TCP_TO_UDP;
                break;
            case 'v':
                verbose = 1; 
                break;
            case 'b':
                strncpy(cfg.bind_addr, optarg, IP_ADDR_MAX - 1);
                break;
            case 'l':
                cfg.bind_port = atoi(optarg);
                break;
            case 's':
                strncpy(cfg.serv_addr, optarg, IP_ADDR_MAX - 1);
                break;
            case 'p':
                cfg.serv_port = atoi(optarg);
                break;
            default:
                usage(argv[0]);
                return -1;
        }
    }

    if (cfg.bind_addr[0] == '\0'
    ||  cfg.serv_addr[0] == '\0'
    ||  cfg.bind_port    <= 0
    ||  cfg.serv_port    <= 0)
    {
        usage(argv[0]);
        return -1;
    }

    vlog ("relay: %s\n", relay_type[type]);
    vlog ("bind:  %s:%d\n", cfg.bind_addr, cfg.bind_port);
    vlog ("serv:  %s:%d\n", cfg.serv_addr, cfg.serv_port);

    if (type == TYPE_UDP_TO_TCP) 
        udp_to_tcp_proxy(&cfg);
    else if (type == TYPE_TCP_TO_UDP) 
        tcp_to_udp_proxy(&cfg);

    return 0;
}
