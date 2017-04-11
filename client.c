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

#include "udp_over_tcp.h"
#include "hash.h"
#include "ezbuf.h"

/*
 * udp client---------|                                 |--------tcp client
 *                    |                                 |
 * udp client-----------(udp server)--(udp to tcp relay)---------tcp client
 *                    |                                 |
 * udp client---------|                                 |--------tcp client
 * 
 */

typedef struct udp_server_ctx
{
    int fd;
    ev_io io;
    ezbuf_t *buf;

    cfg_p cfg;

    hash_t *cache;
}udp_server_ctx_t;

typedef struct tcp_client_ctx 
{
    int fd;
    int connected;
    ev_io recv_io, send_io; 
    ev_timer timeout;
    ezbuf_t *recv_buf, *send_buf;

    char *keystr; //cache keystr
    struct sockaddr_in src_addr;
    socklen_t src_addr_len;

    udp_server_ctx_t *udp_server_ctx;
}tcp_client_ctx_t;

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
    int opt = 1;
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

    return serverfd;
}

static void remove_tcp_client_ctx(EV_P_ tcp_client_ctx_t *tcp_client_ctx)
{
    ev_io_stop(EV_A_ &tcp_client_ctx->recv_io);
    ev_io_stop(EV_A_ &tcp_client_ctx->send_io);
    ev_timer_stop(EV_A_ &tcp_client_ctx->timeout);

    close(tcp_client_ctx->fd);

    ezbuf_del(tcp_client_ctx->recv_buf);
    ezbuf_del(tcp_client_ctx->send_buf);

    //cache del
    hash_del(tcp_client_ctx->udp_server_ctx->cache, tcp_client_ctx->keystr);

    free(tcp_client_ctx);
}

static void tcp_recv_timeout_cb(EV_P_ ev_timer *w, int revents)
{
    tcp_client_ctx_t *tcp_client_ctx;

    tcp_client_ctx = obj_entry(w, tcp_client_ctx_t, timeout);

    vlog("[tcp] timeout!\n");
    remove_tcp_client_ctx(EV_A_ tcp_client_ctx);
}

static void tcp_client_recv_cb(EV_P_ ev_io *w, int revents)
{
    int rx, bsize;
    char buffer[EZBUF_MAX];
    tcp_client_ctx_t *tcp_client_ctx;

    tcp_client_ctx = obj_entry(w, tcp_client_ctx_t, recv_io);

    bsize = sizeof(package_t);

    rx = recv(tcp_client_ctx->fd, buffer, bsize, 0);
    if (rx <= 0)
    {
        vlog("[tcp] recv error!\n");
        remove_tcp_client_ctx(EV_A_ tcp_client_ctx);
        return;
    }

    ev_timer_again(EV_A_ &tcp_client_ctx->timeout);

    ezbuf_put(tcp_client_ctx->recv_buf, buffer, rx);

    while(1){
        if (ezbuf_size(tcp_client_ctx->recv_buf) < bsize) //not enough
            break;

        bsize = ezbuf_get(tcp_client_ctx->recv_buf, buffer, bsize);

        rx = unpack(buffer, bsize, buffer);
        vlog("[tcp] recv(%d)\n", rx);

        //relay
        sendto(tcp_client_ctx->udp_server_ctx->fd, 
            buffer, 
            rx, 
            0, 
            (struct sockaddr *)&tcp_client_ctx->src_addr, 
            tcp_client_ctx->src_addr_len
        );
    }
}

static void tcp_client_send_cb(EV_P_ ev_io *w, int revents)
{
    int r, size, tx;
    char data[EZBUF_MAX];
    struct sockaddr_storage addr;
    socklen_t sock_len = sizeof(addr);
    tcp_client_ctx_t *tcp_client_ctx;

    tcp_client_ctx = obj_entry(w, tcp_client_ctx_t, send_io);

    if (!tcp_client_ctx->connected)
    {
        r = getpeername(tcp_client_ctx->fd, (struct sockaddr *)&addr, &sock_len);
        if (r == 0)
            tcp_client_ctx->connected = 1;
        else
            return;
    }

    size = ezbuf_get(tcp_client_ctx->send_buf, data, EZBUF_MAX);
    if (size == 0)
    {
        ev_io_stop(EV_A, &tcp_client_ctx->send_io);
        return;
    }

    //relay
    tx = send(tcp_client_ctx->fd, data, size, 0);
    if (tx > 0 && tx != size)
    {
        ezbuf_put(tcp_client_ctx->send_buf, data + tx, size - tx);
    }
    if (tx < 0)
    {
        vlog("[tcp] send error\n");
        remove_tcp_client_ctx(EV_A_ tcp_client_ctx);
    }
}

static void udp_server_cb(EV_P_ ev_io *w, int revents)
{
    udp_server_ctx_t *udp_server_ctx;
    tcp_client_ctx_t *tcp_client_ctx;
    size_t rx, bsize; 
    int clientfd;
    char keystr[64], buffer[EZBUF_MAX];
    struct sockaddr_in src_addr;
    socklen_t src_addr_len;

    udp_server_ctx = obj_entry(w, udp_server_ctx_t, io);

    memset(&src_addr, 0, sizeof(struct sockaddr_in));
    src_addr_len = sizeof(struct sockaddr_in);

    rx = recvfrom(udp_server_ctx->fd,
        buffer,
        EZBUF_MAX,
        0,
        (struct sockaddr *)&src_addr,
        &src_addr_len
    );

    if (rx < 0)
        return;

    ezbuf_put(udp_server_ctx->buf, buffer, rx);

    vlog("[udp] recv(%d) from %s:%d\n", rx, inet_ntoa(src_addr.sin_addr), ntohs(src_addr.sin_port));

    memset(keystr, 0, sizeof(keystr));
    sprintf(keystr, "%s:%d", inet_ntoa(src_addr.sin_addr), ntohs(src_addr.sin_port));
    if (hash_has(udp_server_ctx->cache, keystr) == 1)
    {
        vlog("[udp] cache hit: %s\n", keystr);

        tcp_client_ctx = hash_get(udp_server_ctx->cache, keystr);
    }
    else
    {
        vlog("[udp] cache miss: %s\n", keystr);

        //new tcp client ctx
        tcp_client_ctx = (tcp_client_ctx_t *)calloc(1, sizeof(tcp_client_ctx_t));
        assert(tcp_client_ctx);

        tcp_client_ctx->keystr = strdup(keystr);

        tcp_client_ctx->udp_server_ctx = udp_server_ctx; 
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

        ev_timer_init(&tcp_client_ctx->timeout, tcp_recv_timeout_cb, IO_TIMEOUT_MAX, IO_TIMEOUT_MAX);
        ev_timer_start(EV_A_ &tcp_client_ctx->timeout);

        //add key to cache
        hash_set(udp_server_ctx->cache, tcp_client_ctx->keystr, (void *)tcp_client_ctx);
    }

    rx = ezbuf_get(udp_server_ctx->buf, buffer, EZBUF_MAX);
    bsize = packet(buffer, rx, buffer);

    ezbuf_put(tcp_client_ctx->send_buf, buffer, bsize);
    ev_io_start(EV_A, &tcp_client_ctx->send_io);
}

void udp_to_tcp_proxy(cfg_p cfg)
{
    int serverfd;
    struct ev_loop *loop = EV_DEFAULT;
    udp_server_ctx_t *udp_server_ctx = NULL;

    vlog("udp_to_tcp proxy relay\n");

    serverfd = create_udp_server(cfg->bind_addr, cfg->bind_port);
    if (serverfd < 0)
        return;

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
