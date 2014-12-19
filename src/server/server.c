#include <server.h>
#include <socks5-handle.h>
#include <sys/select.h>


static struct ssession *sserver_session_new(int type) {
    
    struct ssession *session = malloc(sizeof(*session));
    if(!session)
        return NULL;
    
    bzero(session, sizeof(session));
    
    session->state = SSESSION_STATE_NONE;
    session->type = type;
    session->dstfd = -1;
    session->srcfd = -1;
    
    session->srcbuf = ringbuffer_new(0);
    if(session->srcbuf == NULL) {
        free(session);
        return NULL;
    }
    
    session->dstbuf = ringbuffer_new(0);
    if(session->dstbuf == NULL) {
        ringbuffer_free(session->srcbuf);
        free(session);
        return NULL;
    }
    
    return session;
}

static void sserver_session_release(struct ssession *session) {
    ringbuffer_free(session->srcbuf);
    ringbuffer_free(session->dstbuf);
    free(session);
}

static int sserver_select_error(struct sserver *server) {
    
    int ret = 0;
    perror("select");
    return ret;
}

static int sserver_select_timeout(struct sserver *server) {
    int ret = 0;
    // printf("%s - %s [%d]\n", __FILE__, __FUNCTION__, __LINE__);
    return ret;
}

static int sserver_do_accept(struct sserver *server) {
    
    int ret = 0;
    int fd = -1;
    struct ssession *session = NULL;
    
    // printf("%s - %s [%d]\n", __FILE__, __FUNCTION__, __LINE__);
    session = sserver_session_new(SSESSION_TYPE_CLIENT);
    if(!session) {
        return -1;
    }
    
    fd = accept(server->fd, &session->srcaddr, (socklen_t *)&session->nsrcaddr);
    if(fd < 0) {
        perror("accept");
        return fd;
    }
    
    session->srcfd = fd;
    session->state = SSESSION_STATE_AUTH_METHOD;
    
    list_add_tail(&(session->node), &server->sessions);
    FD_SET(session->srcfd, &server->readfds);
    if(server->maxfd < session->srcfd)
        server->maxfd = session->srcfd;
    
    return ret;
}

static int sserver_transmit_error(struct sserver *server, struct ssession *session, int err) {
    
    if(err == 0) {
        printf("%s - %s <%d> : peer closed\n", __FILE__, __FUNCTION__, __LINE__);
    } else {
        printf("%s - %s <%d> : %s\n", __FILE__, __FUNCTION__, __LINE__, strerror(err));
    }
    
    if(session->srcfd > 0) {
        shutdown(session->srcfd, SHUT_RDWR);
    }
    
    if(session->dstfd > 0) {
        shutdown(session->dstfd, SHUT_RDWR);
    }
    return 0;
}

static inline int sserver_recv_error(struct sserver *server, struct ssession *session, int err) {
    return sserver_transmit_error(server, session, err);
}

static inline int sserver_send_error(struct sserver *server, struct ssession *session, int err) {
    return sserver_transmit_error(server, session, err);
}

static int sserver_io_read_event(struct sserver *server, fd_set *readfds, struct ssession *session, int iotype) {
    
    int fd = -1;
    struct ringbuffer *rb;
    int bcontinue = 0;
    int n = 0;
    
    // printf("%s - %s [%d]\n", __FILE__, __FUNCTION__, __LINE__);
    switch(iotype) {
        case SSESSION_IO_TYPE_SRC:
            fd = session->srcfd;
            rb = session->dstbuf;
            break;
        case SSESSION_IO_TYPE_DST:
            fd = session->dstfd;
            rb = session->srcbuf;
            break;
        default:
            break;
    }
    
    if(FD_ISSET(fd, readfds)) {
        n = recv_to_ringbuffer(fd, rb);
        if(n > 0) {
            n = server->handle->on_recv(server->handle, session, iotype);
            if(n != SE_NEEDMORE && n != 0) {
                bcontinue = 1;
            }
        } else {
            if((errno == EINTR || errno == EAGAIN)) {
            
            } else {
                sserver_recv_error(server, session, n == 0 ? 0 : errno);
                bcontinue = 1;
            }
        }
    }
    
    return bcontinue;
}

static int sserver_io_write_event(struct sserver *server, fd_set *writefds, struct ssession *session, int iotype) {
    
    int n = 0;
    int fd = -1;
    int bcontinue = 0;
    
    switch(iotype) {
        case SSESSION_IO_TYPE_SRC:
            fd = session->srcfd;
            break;
        case SSESSION_IO_TYPE_DST:
            fd = session->dstfd;
            break;
        default:
            break;
    }
    
    if(FD_ISSET(fd, writefds)) {
        n = server->handle->on_send(server->handle, session, iotype);
        if(n > 0) {
        } else if(errno == EINTR || errno == EAGAIN) {
        } else {
            sserver_send_error(server, session, n == 0 ? 0 : errno);
            bcontinue = 1;
        }
    }
    
    return bcontinue;
}

static int sserver_select_event(struct sserver *server, fd_set *readfds, fd_set *writefds) {
    
    int ret = 0;
    struct list_head *entry;
    struct ssession *session;
    int bcontinue = 0;
    
    list_for_each(entry, &server->sessions) {
        
        session = list_entry(entry, struct ssession, node);
        if(session == NULL) {
            break;
        }
        
        if(session->srcfd > 0 && (bcontinue = sserver_io_read_event(server, readfds, session, SSESSION_IO_TYPE_SRC))) {
            
            FD_CLR(session->srcfd, &server->readfds);
            FD_CLR(session->srcfd, &server->writefds);
            
            FD_CLR(session->dstfd, &server->readfds);
            FD_CLR(session->dstfd, &server->writefds);
            
            list_del(entry);
            sserver_session_release(session);
            continue;
        }
        
        if(session->srcfd > 0 && (bcontinue = sserver_io_write_event(server, writefds, session, SSESSION_IO_TYPE_SRC))) {
            
            FD_CLR(session->srcfd, &server->readfds);
            FD_CLR(session->srcfd, &server->writefds);
            
            FD_CLR(session->dstfd, &server->readfds);
            FD_CLR(session->dstfd, &server->writefds);
            
            list_del(entry);
            sserver_session_release(session);
            continue;
        }
        
        if(session->dstfd > 0 && (bcontinue = sserver_io_read_event(server, readfds, session, SSESSION_IO_TYPE_DST))) {
            
            FD_CLR(session->srcfd, &server->readfds);
            FD_CLR(session->srcfd, &server->writefds);
            
            FD_CLR(session->dstfd, &server->readfds);
            FD_CLR(session->dstfd, &server->writefds);
            
            list_del(entry);
            sserver_session_release(session);
            continue;
        }
        
        if(session->dstfd > 0 && (bcontinue = sserver_io_write_event(server, writefds, session, SSESSION_IO_TYPE_DST))) {
            
            FD_CLR(session->srcfd, &server->readfds);
            FD_CLR(session->srcfd, &server->writefds);
            
            FD_CLR(session->dstfd, &server->readfds);
            FD_CLR(session->dstfd, &server->writefds);
            
            list_del(entry);
            sserver_session_release(session);
            continue;
        }
        
    }
    
    return ret;
}


static int sserver_select(struct sserver *server) {
    
    fd_set readfds;
    fd_set writefds;
    struct timeval tv;
    int err = 0;
    
    struct ssession *session = sserver_session_new(SSESSION_TYPE_SERVER);
    if(!session) {
        return -1;
    }
    
    session->srcfd = server->fd;
    FD_SET(server->fd, &server->readfds);
    server->maxfd = server->fd;
    
    while(server->running) {
        
        FD_COPY(&server->readfds, &readfds);
        FD_COPY(&server->writefds, &writefds);
        memcpy(&tv, &server->tv, sizeof(server->tv));
        
        int nselect = select(server->maxfd + 1, &readfds, &writefds, NULL, &tv);
        if(nselect < 0) {
            sserver_select_error(server);
            return -1;
        } else if(nselect == 0) {
            sserver_select_timeout(server);
        } else {
            // printf("%s - %s [%d]\n", __FILE__, __FUNCTION__, __LINE__);
            if(FD_ISSET(server->fd, &readfds)) {
                err = sserver_do_accept(server);
            } else {
                err = sserver_select_event(server, &readfds, &writefds);
            }
        }
    }
    
    return 0;
}

static int sserver_start(struct sserver *server) {
    
    int ret = 0;
    
    // printf("%s - %s [%d]\n", __FILE__, __FUNCTION__, __LINE__);
    server->fd = tcp_socket_create(0);
    if(server->fd < 0)
        return server->fd;
    
    ret = tcp_socket_bind(server->fd, server->config->host, server->config->port);
    if(ret < 0)
        return ret;
    
    ret = tcp_socket_listen(server->fd, server->config->backlog);
    if(ret < 0)
        return ret;
    
    // printf("%s - %s [%d]\n", __FILE__, __FUNCTION__, __LINE__);
    ret = sserver_select(server);
    
    return ret;
}

static int sserver_stop(struct sserver *server) {
    
    int ret = 0;
    
    return ret;
}

struct sserver_config *sserver_config_new(const char *host, const uint16_t port, const int backlog) {
    
    struct sserver_config *config = malloc(sizeof(*config));
    
    if(!config)
        return NULL;
    
    strncpy(config->host, host, 255);
    config->port = port;
    config->backlog = backlog;
    
    return config;
}

void sserver_config_release(struct sserver_config *config) {
    
    if(config) {
        free(config);
    }
    
}


struct sserver *sserver_new(struct sserver_config *config) {
    
    struct sserver *server = malloc(sizeof(*server));
    
    if(!server)	
        return NULL;
    
    //
    bzero(server, sizeof(*server));
    
    //
    server->fd = -1;
    server->maxfd = -1;
    server->config = config;
    server->running = 1;
    server->tv.tv_sec = 1;
    server->tv.tv_usec = 0;
    
    //
    // LIST_HEAD_INIT(server->sessions);
    INIT_LIST_HEAD(&server->sessions);
    //
    FD_ZERO(&server->writefds);
    FD_ZERO(&server->readfds);
    
    // 
    server->start = sserver_start;
    server->stop = sserver_stop;
    
    server->handle = create_socks5_handle(server);
    return server;
}

void sserver_release(struct sserver *server) {
    
    if(server) {
        if(server->handle) {
            free(server->handle);
        }
        free(server);
    }
}