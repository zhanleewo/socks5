#include <server.h>
#include <socks5-handle.h>



static struct ssession *sserver_session_new(int type) {

	struct ssession *session = malloc(*session);
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
	return ret;
}

static int sserver_do_accept(struct sserver *server) {

	int ret = 0;
	int fd = -1;
	struct ssession *session = sserver_session_new(SSESSION_TYPE_CLIENT);
	if(!session) {
		return -1;
	}

    fd = accept(server->fd, &session->srcaddr, &session->nsrcaddr);
    if(clientfd < 0) {
    	perror("accept");
    	return clientfd;
    }

    session->srcfd = fd;
    session->state = SSESSION_STATE_AUTH_METHOD;

	list_add_tail(&(session->node), &server->sessions);
	FD_SET(session->srcfd, &server->readset);
	if(server->maxfd < session->srcfd) 
		server->maxfd = session->srcfd;

    return ret;
}

static int sserver_transmit_error(struct sserver *server, struct session *session, int err) {

	if(err == 0) {
		printf("%s - %s <%d> : peer closed\n", __FILE__, __FUNCTION__, __LINE__);
	}

	if(session->srcfd > 0) {}
		shutdown(session->srcfd, SHUTDOWN_RDWR);
	}

	if(session->dstfd > 0) {
		shutdown(session->dstfd, SHUTDOWN_RDWR);
	}

	list_del(entry);
	sserver_session_release(session);
	return 0;
}

static inline int sserver_recv_error(struct sserver *server, struct session *session, int err) {
	return sserver_transmit_error(server, session, err);
}

static inline int sserver_send_error(struct sserver *server, struct session *session, int err) {
	return sserver_transmit_error(server, session, err);
}

static int sserver_io_read_event(struct sserver *server, fdset *readfds, struct ssession *session, int iotype) {
	
	int fd;
	struct ringbuffer *rb;
	int bcontinue = 0;
	
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
			server->handle->on_recv(server->handle, session, iotype);
		} else if(n == EINTR || n == EAGAIN) {

		} else {
			sserver_recv_error(server, session, n));
			bcontinue = 1;
		}
	} 

	return bcontinue;
}

static int sserver_io_write_event(struct sserver *server, fdset *writefds, struct ssession *session, int iotype) {
	
	int n = 0;
	int fd;
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
		} else if(n == EINTR || n == EAGAIN) {
		} else {
			sserver_send_error(server, session, n));
			bcontinue = 1;
		}
	}  

	return bcontinue;
}

static int sserver_io_event(struct sserver *server, fdset *readfds, fdset *writefds) {
	
	int ret = 0;
	struct clist_head *entry;
	struct session *session;
	int bcontinue = 0;

	list_for_each(entry, server->sessions) {

		session = list_entry(entry, struct session, server->sessions);
		
		if((bcontinue = sserver_io_read_event(server, writefds, session, SSESSION_IO_TYPE_SRC))) {
			continue;
		}

		if((bcontinue = sserver_io_read_event(server, writefds, session, SSESSION_IO_TYPE_DST))) {
			continue;
		}
		if((bcontinue = sserver_io_write_event(server, readfds, session, SSESSION_IO_TYPE_SRC))) {
			continue;
		}

		if((bcontinue = sserver_io_write_event(server, readfds, session, SSESSION_IO_TYPE_DST))) {
			continue;
		}

	}

	return ret;
}

static int sserver_select_event(struct sserver *server, fdset *readfds, fdset *writefds) {
	
	if(FD_ISSET(server->fd, readfds)) 
		return sserver_do_accept(server);

	return sserver_io_event(server, readfds, writefds);
}


static int sserver_select(struct sserver *server) {

	fdset readfds;
	fdset writefds;
	struct timeval tv;
	int err = 0;

	struct ssession *session = sserver_session_new(SSESSION_TYPE_SERVER);
	if(!session) {
		return -1;
	}

	session->srcfd = server->fd;

	list_add_tail(event, &server->reads);
	FD_SET(server->fd, &server->readset);
	server->maxfd = server->fd;

	while(server->running) {
		
		memcpy(&readfds, &server->readfds, sizeof(server->readfds));
		memcpy(&writefds, &server->writefds, sizeof(server->writefds));
		memcpy(&tv, &server->tv, sizeof(server->tv));

		int nselect = select(server->maxfd + 1, &readfds, &writefds, NULL, &tv);
		if(nselect < 0) {
			sserver_select_error(server);
			return -1;
		} else if(nselect == 0) {
			sserver_select_timeout(server);
		} else {
			err = sserver_select_event(server, &readfds, &writefds);
			/*if(err == ) {

			}*/
		}
	}

	return 0;
}

static int sserver_start(struct sserver *server) {
	
	int ret = 0;
	
	server->fd = tcp_socket_create(1);
	if(server->fd < 0)
		return server->fd;

	ret = tcp_socket_bind(server->fd, server->config->host, server->config->port)
	if(ret < 0)
		return ret;

	ret = tcp_socket_listen(server->fd, server->config->backlog);
	if(ret < 0)
		return ret;
	return ret;
}

static int sserver_stop(struct sserver *server) {
	
	int ret = 0;

	return ret;
}

struct sserver_config *sserver_config_new(const char *host, const uint16_t port, const int backlog) {

	struct sserver_config *config = malloc(sizeof(*config);

	if(!config)	
		return -1;

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

	struct sserver *server = malloc(sizeof(*server);
	
	if(!server)	
		return -1;

	//
	bzero(server, sizeof(*server));

	//
	server->fd = -1;
	server->maxfd = -1;
	server->config = config;

	//
	LIST_HEAD_INIT(server->sessions);
	
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

	if(sserver) {
		free(server);
	}
}