#ifndef COM_LINZHAN_COMMON_COMMON_H_
#define COM_LINZHAN_COMMON_COMMON_H_

#include <fcntl.h>
#include <errno.h>
#include <stdio.h>

int tcp_socket_create(int blocked);
int tcp_socket_bind(int sockfd, char *host, uint16_t port);
int tcp_socet_listen(int sockfd, int backlog);

int tcp_socket_connect_with_ip(uint32_t ip, uint16_t port);
int tcp_socket_connect_with_domain(char *domin, uint16_t port);

int recv_to_ringbuffer(int sockfd, struct ringbuffer *rb);
#endif
