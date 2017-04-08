/*
 * =====================================================================================
 *
 *       Filename:  udp_2_tcp.c
 *
 *    Description:  udp to tcp relay
 *
 *        Version:  1.0
 *        Created:  2017年04月08日 15时17分12秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  lizhixian (T3), lizhixian@integritytech.com.cn
 *   Organization:  T3
 *
 * =====================================================================================
 */

#include "iproxy.h"
#include "hash.h"
#include "ezbuf.h"

typedef struct tcp_client_ctx 
{
    int fd;
    ev_io recv_io; 
    ev_io send_io; 
    ev_io *udp_io;
    struct sockaddr_in src_addr;
    socklen_t src_addr_len;

    ezbuf_t *recv_buf, *send_buf;
}tcp_client_ctx_t;

typedef struct udp_server_ctx
{
    int fd;
    ev_io io;
    hash_t *cache;
    ezbuf_t *buf;
    cfg_p cfg;
}udp_server_ctx_t;

static int create_tcp_client(const char *ip, int port)
{
    int rc, clientfd;
    struct sockaddr_in saddr;
    socklen_t saddr_len;

    clientfd = socket(AF_INET, SOCK_STREAM, 0);
    if (clientfd < 0)
    {
        vlog("tcp client error\n");
        return -1;
    }

    memset(&saddr, 0, sizeof(saddr));
    saddr.sin_family      = AF_INET;
    saddr.sin_port        = htons(port);
    saddr.sin_addr.s_addr = inet_addr(ip);

    saddr_len = sizeof(saddr);

    fcntl(clientfd, F_SETFL, fcntl(clientfd, F_GETFL) | O_NONBLOCK);

    rc = connect(clientfd, (struct sockaddr *)&saddr, saddr_len);
    if (rc == -1 && errno != EINPROGRESS)
    {
        vlog("tcp client connect error\n");
        return -1;
    }

    return clientfd;
}

static int create_udp_server(const char *ip, int port)
{
    int rc, serverfd; 
    struct sockaddr_in addr;
    socklen_t addr_len;

    //create udp server
    serverfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (serverfd < 0)
    {
        vlog("udp socket error\n");
        return -1;
    }

    memset(&addr, 0, sizeof(struct sockaddr_in));
    addr.sin_family        = AF_INET;
    addr.sin_port          = htons(port);
    addr.sin_addr.s_addr   = inet_addr(ip);

    addr_len = sizeof(addr);

    rc = bind(serverfd, (struct sockaddr *)&addr, addr_len);
    if (rc != 0)
    {
        vlog("udp bind error\n");
        return -1;
    }

    return serverfd;
}

static void remove_tcp_client_ctx(EV_P_ tcp_client_ctx_t *tcp_client_ctx)
{
    ev_io_stop(EV_A, &tcp_client_ctx->recv_io);
    ev_io_stop(EV_A, &tcp_client_ctx->send_io);

    close(tcp_client_ctx->fd);

    ezbuf_del(tcp_client_ctx->recv_buf);
    ezbuf_del(tcp_client_ctx->send_buf);
}

static void tcp_client_recv_cb(EV_P_ ev_io *w, int revents)
{
    int rx;
    tcp_client_ctx_t *tcp_client_ctx;

    tcp_client_ctx = obj_entry(w, tcp_client_ctx_t, recv_io);

    rx = recv(tcp_client_ctx->fd, tcp_client_ctx->recv_buf->buffer, EZBUF_MAX, 0);
    if (rx <= 0)
    {
        remove_tcp_client_ctx(EV_A, tcp_client_ctx);
        return;
    }

    sendto(tcp_client_ctx->udp_io->fd, 
        tcp_client_ctx->recv_buf->buffer, 
        rx, 
        0, 
        (struct sockaddr *)&tcp_client_ctx->src_addr, 
        tcp_client_ctx->src_addr_len);
}

static void tcp_client_send_cb(EV_P_ ev_io *w, int revents)
{
    int size, tx;
    char data[EZBUF_MAX];
    tcp_client_ctx_t *tcp_client_ctx;

    tcp_client_ctx = obj_entry(w, tcp_client_ctx_t, send_io);

    size = ezbuf_get(tcp_client_ctx->send_buf, data, EZBUF_MAX);
    if (size == 0)
    {
        ev_io_stop(EV_A, &tcp_client_ctx->send_io);
        return;
    }

    tx = send(tcp_client_ctx->send_io.fd, data, size, 0);
    if (tx > 0 && tx != size)
        ezbuf_put(tcp_client_ctx->send_buf, data + tx, size - tx);
    if (tx < 0)
        remove_tcp_client_ctx(EV_A, tcp_client_ctx);
}

static void udp_server_cb(EV_P_ ev_io *w, int revents)
{
    udp_server_ctx_t *udp_server_ctx;
    tcp_client_ctx_t *tcp_client_ctx;
    size_t rx; 
    int clientfd;
    char key[64];
    struct sockaddr_in src_addr;
    socklen_t src_addr_len;

    udp_server_ctx = obj_entry(w, udp_server_ctx_t, io);

    memset(&src_addr, 0, sizeof(struct sockaddr_in));
    src_addr_len = sizeof(struct sockaddr_in);

    rx = recvfrom(udp_server_ctx->fd,
        udp_server_ctx->buf->buffer,
        EZBUF_MAX,
        0,
        (struct sockaddr *)&src_addr,
        &src_addr_len
    );

    vlog("[udp] recv(%d) from %s:%d\n", rx, inet_ntoa(src_addr.sin_addr), ntohs(src_addr.sin_port));

    memset(key, 0, sizeof(key));
    snprintf(key, sizeof(key) - 1, "%s:%d", inet_ntoa(src_addr.sin_addr), ntohs(src_addr.sin_port));
    if( hash_has(udp_server_ctx->cache, key) )
    {
        vlog("[udp] cache hit: %s\n", key);

        tcp_client_ctx = hash_get(udp_server_ctx->cache, key);
    }
    else
    {
        vlog("[udp] cache miss: %s\n", key);

        //new tcp client ctx
        tcp_client_ctx = (tcp_client_ctx_t *)calloc(1, sizeof(tcp_client_ctx_t));
        assert(tcp_client_ctx);

        tcp_client_ctx->udp_io = w; 
        tcp_client_ctx->src_addr = src_addr;
        tcp_client_ctx->src_addr_len = src_addr_len;
        
        tcp_client_ctx->recv_buf = ezbuf_new();
        assert(tcp_client_ctx->recv_buf);

        tcp_client_ctx->send_buf = ezbuf_new();
        assert(tcp_client_ctx->send_buf);

        //create tcp client
        clientfd = create_tcp_client(udp_server_ctx->cfg->serv_addr, udp_server_ctx->cfg->serv_port);
        if (clientfd < 0)
        {
            free(tcp_client_ctx);
            return;
        }

        tcp_client_ctx->fd = clientfd;

        ev_io_init(&tcp_client_ctx->recv_io, tcp_client_recv_cb, clientfd, EV_READ);
        ev_io_init(&tcp_client_ctx->send_io, tcp_client_send_cb, clientfd, EV_WRITE);

        ev_io_start(EV_A, &tcp_client_ctx->recv_io);

        //add key to cache
        hash_set(udp_server_ctx->cache, key, (void *)tcp_client_ctx);
    }

    ezbuf_put(tcp_client_ctx->send_buf, udp_server_ctx->buf->buffer, rx);
    ev_io_start(EV_A, &tcp_client_ctx->send_io);
}

void udp_to_tcp_proxy(cfg_p cfg)
{
    int serverfd;
    struct ev_loop *loop = EV_DEFAULT;
    udp_server_ctx_t *udp_server_ctx = NULL;

    vlog("udp_to_tcp proxy relay\n");

    serverfd = create_udp_server(cfg->bind_addr, cfg->bind_port);

    //udp server init 
    udp_server_ctx = (udp_server_ctx_t *)calloc(1, sizeof(udp_server_ctx_t));
    assert(udp_server_ctx);

    udp_server_ctx->cache = hash_new();
    assert(udp_server_ctx->cache);

    udp_server_ctx->buf = ezbuf_new();
    assert(udp_server_ctx->buf);

    udp_server_ctx->cfg = cfg;
    udp_server_ctx->fd = serverfd;

    ev_io_init(&udp_server_ctx->io, udp_server_cb, serverfd, EV_READ);
    ev_io_start(loop, &udp_server_ctx->io);

    //dispatch
    ev_run(loop, 0);
}
