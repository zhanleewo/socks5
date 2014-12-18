#include <common.h>

#define RECV_BUFFER_SIZE	4096

int tcp_socket_create(int blocked) {
	
	int fd = socket(AF_INET, SOCK_STREAM, 0);
	if(blocked) {
		set_socket_blocked(sockfd);
	} else {
		set_socket_nonblock(sockfd);
	}

	return fd;
}

int tcp_socket_connect(char *ip, uint16_t port) {
	
	struct sockaddr_in addr;
	int ret = 0;
	int fd = tcp_socket_create(0);
	if((ret = connect(fd, &addr, sizeof(addr))) != 0) {

	}
	return fd;
}


int tcp_socket_bind(int sockfd, char *host, uint16_t port) {
	
	int reuse = 1;
	struct sockaddr_in addr;

	int ret = setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
	if(ret < 0) {
		perror("setsockopt");
		return ret;
	}

	bzero(&addr, sizeof(addr));

	ret = bind(sockfd, &addr, sizeof(addr), );
	if(ret < 0) {
		perror("bind");
	}

	return ret;
}

int tcp_socet_listen(int sockfd, int backlog) {

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

	int n = 0;
	int count = 0;
	char buf[RECV_BUFFER_SIZE];
	int skip = 0;

	while(!skip) {
		n = recv(session->srcfd, buf, RECV_BUFFER_SIZE, 0);
		if(n > 0) {
			count += n;
			ringbuffer_write(session->dstbuf, buf, n);
		} else {
			count = n;
			skip = (n <= 0);
		}
	}

	return count;
}
