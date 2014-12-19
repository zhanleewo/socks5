#include <common.h>

#define RECV_BUFFER_SIZE	4096

int tcp_socket_create(int blocked) {
    
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if(blocked) {
        set_socket_blocked(fd);
    } else {
        set_socket_nonblock(fd);
    }
    
    return fd;
}

int tcp_socket_connect_with_ip(uint32_t ip, uint16_t port) {
    
    struct sockaddr_in addr;
    int fd = -1;
    int ret = -1;
    
    bzero(&addr, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = ip;
    addr.sin_port = port;
    addr.sin_len = sizeof(addr);
    fd = tcp_socket_create(1);
    if(fd < 0) {
        return fd;
    }
    
    if((ret = connect(fd, (struct sockaddr *) &addr, sizeof(addr))) != 0) {
        return ret;
    }
    set_socket_nonblock(fd);
    
    return fd;
}

int tcp_socket_connect_with_domain(char *domin, uint16_t port) {
    
    struct sockaddr_in addr;
    int ret = -1;
    int fd = -1;
    
    struct addrinfo *first, *iter, hints;
    
    memset(&hints, 0, sizeof(hints));
    
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = 0;
    hints.ai_flags = 0;
    
    getaddrinfo(domin, NULL, &hints, &first);
    if(first == NULL) {
        return 0;
    }
    
    fd = tcp_socket_create(1);
    if(fd < 0) {
        return fd;
    }
    
    for (iter = first; iter->ai_next != NULL; iter = iter->ai_next) {
        memcpy(&addr, iter->ai_addr, iter->ai_addrlen);
        addr.sin_port = port;
        
        if((ret = connect(fd, (struct sockaddr *) &addr, sizeof(addr))) == 0) {
            break;
        }
    }
    freeaddrinfo(first);
    
    if(ret != 0) {
        return ret;
    }
    
    if(ret == 0 && fd > 0) {
        set_socket_nonblock(fd);
    }
    
    return fd;
}

int tcp_socket_bind(int sockfd, char *host, uint16_t port) {
    
    int reuse = 1;
    struct sockaddr_in addr;
    bzero(&addr, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(host);
    addr.sin_port = htons(port);
    
    int ret = setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
    if(ret < 0) {
        perror("setsockopt");
        return ret;
    }
    
    ret = bind(sockfd, (struct sockaddr *) &addr, sizeof(addr));
    if(ret < 0) {
        perror("bind");
    }
    
    return ret;
}

int tcp_socket_listen(int sockfd, int backlog) {
    
    int ret = listen(sockfd, backlog);
    if(ret < 0) {
        perror("listen");
    }
    return ret;
}

int set_socket_nonblock(int sockfd) {
    int opts;
    
    opts = fcntl(sockfd, F_GETFL);
    if (opts < 0) {
        perror("fcntl(F_GETFL)");
        return -1;
    }
    
    opts = (opts | O_NONBLOCK);
    if (fcntl(sockfd, F_SETFL, opts) < 0) {
        perror("fcntl(F_SETFL)");
        return -1;
    }
    
    return 0;
}

int set_socket_blocked(int sockfd) {
    
    int opts = fcntl(sockfd, F_GETFL);
    if(opts < 0) {
        perror("fcntl(F_GETFL)");
        return -1;
    }
    
    if (opts & O_NONBLOCK) {
        opts &= ~O_NONBLOCK;
        if (fcntl(sockfd, F_SETFL, opts) < 0) {
            perror("fcntl(F_SETFL)");
        }
        return 0;
    }
    return 0;
}

int recv_to_ringbuffer(int sockfd, struct ringbuffer *rb) {
    
    int nread = 0;
    int nwrite = 0;
    int count = 0;
    int skip = 0;
    int nmaxread = ringbuffer_can_write(rb);
    char buf[RECV_BUFFER_SIZE];
    
    while(!skip) {
        
        int nneedread = nmaxread > RECV_BUFFER_SIZE ? RECV_BUFFER_SIZE : nmaxread;
        
        nread = recv(sockfd, buf, nneedread, 0);
        if(nread > 0) {
            count += nread;
            nmaxread -= nread;
            nwrite = ringbuffer_write(rb, buf, nread);
            if(nmaxread == 0) {
                break;
            }
        } else {
            if(errno == EAGAIN || errno == EINTR) {
                break;
            }
            count = errno;
            skip = (nread <= 0);
        }
    }
    
    return count;
}
