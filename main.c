/*
 * =====================================================================================
 *
 *       Filename:  main.c
 *
 *    Description:  udp over tcp 
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

#include "udp_over_tcp.h"

const char *relay_type[] = {"udp_to_tcp", "tcp_to_udp"};

extern int verbose;

static void usage(char *name)
{
    printf(
        "usage: \n"
        "\t%s -v -t u2t -b 0.0.0.0 -l 4500 -s 192.168.1.111 -p 4500\n"
        "\t* u2t udp to tcp\n"
        "\t* t2u tcp to udp\n",
        basename(name)
    );
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
