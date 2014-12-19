#ifndef COM_LINZHAN_SERVER_SSERVER_H_
#define COM_LINZHAN_SERVER_SSERVER_H_


#include <common.h>
#include <list.h>
#include <ringbuffer.h>

#define SSESSION_TYPE_SERVER	0x1
#define SSESSION_TYPE_CLIENT	0x2

#define SSESSION_IO_TYPE_DST	0x1
#define SSESSION_IO_TYPE_SRC	0x2

#define SSESSION_STATE_NONE				0x0
#define SSESSION_STATE_AUTH_METHOD		0x1
#define SSESSION_STATE_AUTH_BEIGIN		0x2
#define SSESSION_STATE_CONNECT			0x3
#define SSESSION_STATE_TRANSMIT			0x4


#define SE_UNSUPPORT_VERSION    -0x01
#define SE_UNSUPPORT_METHOD     -0x02
#define SE_NEEDMORE             -0x03
#define SE_NO_METHOD            -0x04

struct sserver_handle;

struct sserver_config {
    char host[256];
    uint16_t port;
    int backlog;
};

struct ssession {
    
    struct list_head node;
    
    
    int type;
    int state;
    
    int srcfd;
    struct sockaddr srcaddr;
    int nsrcaddr;
    struct ringbuffer *srcbuf;
    
    int dstfd;
    struct sockaddr dstaddr;
    int ndstaddr;
    struct ringbuffer *dstbuf;
    
};

struct sserver {
    
    int fd;
    int maxfd;
    int running;
    struct timeval tv;
    
    struct sserver_config *config;
    
    struct list_head sessions;
    
    fd_set readfds;
    fd_set writefds;
    
    int (*start)(struct sserver *server);
    int (*stop)(struct sserver *server);
    
    struct sserver_handle *handle;
};


struct sserver_handle {
    struct sserver *server;
    int (*on_recv)(struct sserver_handle *handle, struct ssession *session, int iotype);
    int (*on_send)(struct sserver_handle *handle, struct ssession *session, int iotype);
};

struct sserver_config *sserver_config_new(const char *host, const uint16_t port, const int backlog);
void sserver_config_release(struct sserver_config *config);

struct sserver *sserver_new(struct sserver_config *config);
void sserver_release(struct sserver *server);

#endif