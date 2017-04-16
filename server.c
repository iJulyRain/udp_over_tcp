/*
 * =====================================================================================
 *
 *       Filename:  tcp_2_udp.c
 *
 *    Description:  tcp to udp relay
 *
 *        Version:  1.0
 *        Created:  2017年04月08日 15时17分27秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  lizhixian (T3), lizhixian@integritytech.com.cn
 *   Organization:  T3
 *
 * =====================================================================================
 */

#include "udp_over_tcp.h"
#include "hash.h"
#include "ezbuf.h"
#include "cipher.h"

/*
 * tcp client---------|                                |--------udp client
 *                    |                                |
 * tcp client----------(tcp server)--(tcp to udp relay)---------udp client
 *                    |                                |
 * tcp client---------|                                |--------udp client
 * 
 */

typedef struct relay_ctx 
{
    int tcp_fd;
    ev_io recv_io, send_io;
    ev_timer timeout;
    ezbuf_t *recv_buf, *send_buf;
    struct sockaddr_in src_addr;
    socklen_t src_addr_len;

    int udp_fd;
    ev_io udp_io;

    cfg_p cfg;
}relay_ctx_t;

typedef struct tcp_server_ctx
{
    int fd;
    ev_io io;

    cfg_p cfg;
}tcp_server_ctx_t;

static int create_tcp_server(const char *ip, int port)
{
    int opt = 1;
    int rc, serverfd; 
    struct sockaddr_in addr;
    socklen_t addr_len;

    //create udp server
    serverfd = socket(AF_INET, SOCK_STREAM, 0);
    if (serverfd < 0)
    {
        vlog("udp socket error\n");
        return -1;
    }

    setsockopt(serverfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

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

    rc = listen(serverfd, 32);
    if (rc != 0)
    {
        vlog("udp listen error\n");
        return -1;
    }

    return serverfd;
}

static void remove_relay_ctx(EV_P_ relay_ctx_t *relay_ctx)
{
    ev_io_stop(EV_A_ &relay_ctx->recv_io);
    ev_io_stop(EV_A_ &relay_ctx->send_io);
    ev_io_stop(EV_A_ &relay_ctx->udp_io);
    ev_timer_stop(EV_A_ &relay_ctx->timeout);

    close(relay_ctx->tcp_fd);
    close(relay_ctx->udp_fd);

    ezbuf_del(relay_ctx->recv_buf);
    ezbuf_del(relay_ctx->send_buf);

    free(relay_ctx);
}

static void tcp_recv_timeout_cb(EV_P_ ev_timer *w, int revents)
{
    relay_ctx_t *relay_ctx;

    relay_ctx = obj_entry(w, relay_ctx_t, timeout);

    vlog("[tcp] timeout!\n");
    remove_relay_ctx(EV_A, relay_ctx);
}

static void tcp_recv_cb(EV_P_ ev_io *w, int revents)
{
    int rx, bsize;
    char buffer[EZBUF_MAX];
    struct sockaddr_in saddr;
    socklen_t saddr_len;

    relay_ctx_t *relay_ctx;
    relay_ctx = obj_entry(w, relay_ctx_t, recv_io);

    bsize = sizeof(package_t);

    rx = recv(relay_ctx->tcp_fd, buffer, bsize, 0);
    if (rx <= 0)
    {
        remove_relay_ctx(EV_A, relay_ctx);
        return;
    }

    ev_timer_again(EV_A_ &relay_ctx->timeout);

    ezbuf_put(relay_ctx->recv_buf, buffer, rx);

    memset(&saddr, 0, sizeof(saddr));
    saddr.sin_family = AF_INET;
    saddr.sin_port = htons(relay_ctx->cfg->serv_port);;
    saddr.sin_addr.s_addr = inet_addr(relay_ctx->cfg->serv_addr);
    saddr_len = sizeof(saddr);

    while(1){
        if (ezbuf_size(relay_ctx->recv_buf) < bsize)
            break;

        bsize = ezbuf_get(relay_ctx->recv_buf, buffer, bsize);
        rx = unpack(buffer, bsize, buffer);

        rs_decrypt(buffer, buffer, rx, relay_ctx->cfg->password);

        vlog("[tcp] recv(%d) from %s:%d\n", rx, inet_ntoa(relay_ctx->src_addr.sin_addr), ntohs(relay_ctx->src_addr.sin_port));

        sendto(
            relay_ctx->udp_fd,
            buffer,
            rx,
            0,
            (struct sockaddr *)&saddr,
            saddr_len
        );
    }
}

static void tcp_send_cb(EV_P_ ev_io *w, int revents)
{
    int size, tx;
    char data[EZBUF_MAX];
    relay_ctx_t *relay_ctx;

    relay_ctx = obj_entry(w, relay_ctx_t, send_io);

    size = ezbuf_get(relay_ctx->send_buf, data, EZBUF_MAX);
    if (size == 0)
    {
        ev_io_stop(EV_A, &relay_ctx->send_io);
        return;
    }

    //relay
    tx = send(relay_ctx->send_io.fd, data, size, 0);
    if (tx > 0 && tx != size)
        ezbuf_put(relay_ctx->send_buf, data + tx, size - tx);
    if (tx < 0)
        remove_relay_ctx(EV_A, relay_ctx);
}

static void udp_recv_cb(EV_P_ ev_io *w, int revents)
{
    int rx, bsize;
    char buffer[EZBUF_MAX];
    relay_ctx_t *relay_ctx;
    struct sockaddr_in src_addr;
    socklen_t src_addr_len;

    relay_ctx = obj_entry(w, relay_ctx_t, udp_io);

    memset(&src_addr, 0, sizeof(struct sockaddr_in));
    src_addr_len = sizeof(struct sockaddr_in);

    rx = recvfrom(relay_ctx->udp_fd,
        buffer,
        EZBUF_MAX,
        0,
        (struct sockaddr *)&src_addr,
        &src_addr_len
    );

    if (rx < 0)
        return;

    vlog("[udp] recv(%d) from %s:%d\n", rx, inet_ntoa(src_addr.sin_addr), ntohs(src_addr.sin_port));

    rs_encrypt(buffer, buffer, rx, relay_ctx->cfg->password);

    bsize = packet(buffer, rx, buffer);

    ezbuf_put(relay_ctx->send_buf, buffer, bsize);
    ev_io_start(EV_A, &relay_ctx->send_io);
}

static void tcp_server_accept_cb(EV_P_ ev_io *w, int revents)
{
    int acceptfd;
    struct sockaddr_in saddr;
    socklen_t saddr_len;
    relay_ctx_t *relay_ctx;
    tcp_server_ctx_t *tcp_server_ctx;

    tcp_server_ctx = obj_entry(w, tcp_server_ctx_t, io);

    memset(&saddr, 0, sizeof(saddr));
    saddr_len = sizeof(saddr);
    acceptfd = accept(tcp_server_ctx->fd, (struct sockaddr *)&saddr, &saddr_len);
    if (acceptfd < 0)
    {
        vlog("accept error!\n");
        return;
    }

    vlog("[tcp] connect from %s:%d\n", inet_ntoa(saddr.sin_addr), ntohs(saddr.sin_port));

    //new relay ctx
    relay_ctx = (relay_ctx_t *)calloc(1, sizeof(relay_ctx_t));
    assert(relay_ctx);

    relay_ctx->cfg = tcp_server_ctx->cfg;
    relay_ctx->src_addr = saddr;
    relay_ctx->src_addr_len = saddr_len;

    relay_ctx->tcp_fd = acceptfd;
    ev_io_init(&relay_ctx->recv_io, tcp_recv_cb, acceptfd, EV_READ);
    ev_io_init(&relay_ctx->send_io, tcp_send_cb, acceptfd, EV_WRITE);
    ev_io_start(EV_A_ &relay_ctx->recv_io);

    ev_timer_init(&relay_ctx->timeout, tcp_recv_timeout_cb, IO_TIMEOUT_MAX, IO_TIMEOUT_MAX);
    ev_timer_start(EV_A_ &relay_ctx->timeout);

    relay_ctx->recv_buf = ezbuf_new();
    assert(relay_ctx->recv_buf);

    relay_ctx->send_buf = ezbuf_new();
    assert(relay_ctx->send_buf);

    relay_ctx->udp_fd = socket(AF_INET, SOCK_DGRAM, 0);
    assert(relay_ctx->udp_fd > 0);

    ev_io_init(&relay_ctx->udp_io, udp_recv_cb, relay_ctx->udp_fd, EV_READ);
    ev_io_start(EV_A_ &relay_ctx->udp_io);
}

void tcp_to_udp_proxy(cfg_p cfg)
{
    int serverfd;
    struct ev_loop *loop = EV_DEFAULT;
    tcp_server_ctx_t tcp_server_ctx;

    vlog("tcp_to_udp proxy relay\n");

    serverfd = create_tcp_server(cfg->bind_addr, cfg->bind_port);
    if (serverfd < 0)
        return;

    //create tcp server accept
    tcp_server_ctx.fd = serverfd;
    tcp_server_ctx.cfg = cfg;
    ev_io_init(&tcp_server_ctx.io, tcp_server_accept_cb, serverfd, EV_READ);
    ev_io_start(loop, &tcp_server_ctx.io);

    //dispatch
    ev_run(loop, 0);
}
