#ifndef COM_LINZHAN_COMMON_COMMON_H_
#define COM_LINZHAN_COMMON_COMMON_H_

#include <errno.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include <sys/socket.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <fcntl.h>

#include <ringbuffer.h>

int tcp_socket_create(int blocked);
int tcp_socket_bind(int sockfd, char *host, uint16_t port);
int tcp_socket_listen(int sockfd, int backlog);
int set_socket_nonblock(int sockfd);
int set_socket_blocked(int sockfd);

int tcp_socket_connect_with_ip(uint32_t ip, uint16_t port);
int tcp_socket_connect_with_domain(char *domin, uint16_t port);

int recv_to_ringbuffer(int sockfd, struct ringbuffer *rb);
#endif
