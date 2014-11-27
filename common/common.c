#include <common.h>

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
	if (opts & O_NONBLOCK) {
		perror("fcntl(F_GETFL)");
		return -1;
	}
	
	opts &= ~O_NONBLOCK;
	if (fcntl(sockfd, F_SETFL, opts) < 0) {
		perror("fcntl(F_SETFL)");
	}
	return 0;
}
