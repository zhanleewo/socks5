
static int socks5_src_choose_auth_method(struct sserver_handle *handle, struct ssession *session) {
	
	// 1. parse 
	// 2. clear buf;
	// 3. add response to sendbuf;
	// 4. add fd to writefds
	// 5. set session state to SSESSION_STATE_CONNECT

	int ret = 0;
	char ver = 0;
	char nmethod = 0;
	char selmethod = 0x00;

	struct ringbuffer *rb = session->dstbuf;

	nread = ringbuffer_transaction_read(rb, &ver, 1);
	if(nread <= 0) {
		ringbuffer_transaction_rollback(session->srcbuf, &tran);
		return nread;
	}

	if(ver != 0x5) {
		return -1;
	}
    
	nread = ringbuffer_transaction_read(rb, &nmethod, 1);
	if(nread <= 0) {
		ringbuffer_transaction_rollback(session->srcbuf, &tran);
		return nread;
	}

	if(nmethod == 0) {
		return -2;
	}
	
	nread = ringbuffer_transaction_read(rb, &nmethod, (int) nmethod);
	if(nread <= 0 && nread != (int) nmethod) {
		ringbuffer_transaction_rollback(session->srcbuf, &tran);
		return nread;
	}

	ringbuffer_clear(rb);
	ringbuffer_write(rb, &ver, 1);
	ringbuffer_write(rb, &selmethod, 1);

	ret = socks5_src_do_transmit(handle, session);
	
	session->state = SSESSION_STATE_CONNECT;

	return ret;
}

static int socks5_src_choose_do_auth(struct sserver_handle *handle, struct ssession *session) {
	
	// 1. parse
	// 2. clear buf;
	// 3. add response to sendbuf;
	// 4. add fd to writefds
	// 5. set session state to SSESSION_STATE_CONNECT

	int ret = 0;

	return ret;
}

static int socks5_src_do_connect(struct sserver_handle *handle, struct ssession *session) {
	
	// 1. parse
	// 2. clear buf;
	// 3. connect to dest addr
	// 4. set session.dstfd
	// 5. add response to sendbuf;
	// 6. add fd to writefds
	// 7. set session state to SSESSION_STATE_TRANSMIT

	int ret = 0;
	char ver = 0x00;
	char cmd = 0x00;
	char rsv = 0x00;
	char atyp = 0x00;
	char ndstaddr = 0x00;
	uint32_t ip = 0x00;
	char dstaddr[256] = {0x00};
	uint16_t dstport = 0;

	char rep = 0x00;
	char nbndaddr = 0x00;
	char bndaddr[256] = {0x00};
	uint16_t bndport = 0;
	int fd = 0;


    nread = ringbuffer_transaction_read(rb, &ver, 1);
    if(nread <= 0) {
        ringbuffer_transaction_rollback(session->srcbuf, &tran);
        return nread;
    }

    nread = ringbuffer_transaction_read(rb, &cmd, 1);
    if(nread <= 0) {
        ringbuffer_transaction_rollback(session->srcbuf, &tran);
        return nread;
    }

    nread = ringbuffer_transaction_read(rb, &rsv, 1);
    if(nread <= 0) {
        ringbuffer_transaction_rollback(session->srcbuf, &tran);
        return nread;
    }

    nread = ringbuffer_transaction_read(rb, &atyp, 1);
    if(nread <= 0) {
        ringbuffer_transaction_rollback(session->srcbuf, &tran);
        return nread;
    }


    switch(atyp) {
    case 0x01:	// ipv4
    	nread = ringbuffer_transaction_read(rb, &ip, 4);
    	if(nread <= 0) {
    		ringbuffer_transaction_rollback(session->srcbuf, &tran);
        	return nread;
		}

		nread = ringbuffer_transaction_read(rb, &dstport, 2);
		if(nread <= 0) {
			ringbuffer_transaction_rollback(session->srcbuf, &tran);
			return nread;
		}
    	session->dstfd = tcp_socket_connect_with_ip(ip, dstport);
    	break;
    case 0x03:	// domain
		nread = ringbuffer_transaction_read(rb, &ndstaddr, 1);
		if(nread <= 0) {
			ringbuffer_transaction_rollback(session->srcbuf, &tran);
			return nread;
		}

		nread = ringbuffer_transaction_read(rb, dstaddr, ndstaddr);
		if(nread <= 0) {
			ringbuffer_transaction_rollback(session->srcbuf, &tran);
			return nread;
		}

		nread = ringbuffer_transaction_read(rb, &dstport, 2);
		if(nread <= 0) {
			ringbuffer_transaction_rollback(session->srcbuf, &tran);
			return nread;
		}
    	session->dstfd = tcp_socket_connect_with_ip(dstaddr, dstport);
    	break;
    case 0x04:	// ipv6
    	break;
    }

    if(session->dstfd < 0) {
    	/*
    	rep = 0;
		0x00        成功
		0x01        一般性失败
		0x02        规则不允许转发
		0x03        网络不可达
		0x04        主机不可达
		0x05        连接拒绝
		0x06        TTL超时
		0x07        不支持请求包中的CMD
		0x08        不支持请求包中的ATYP
		0x09-0xFF   unassigned
		*/
    }

	ringbuffer_clear(rb);
	ringbuffer_write(rb, &ver, 1);
	ringbuffer_write(rb, &rep, 1);
	ringbuffer_write(rb, &rsv, 1);
	ringbuffer_write(rb, &atyp, 1);
	ringbuffer_write(rb, &nbndaddr, 1);
	ringbuffer_write(rb, bndaddr, nbndaddr);
	ringbuffer_write(rb, &bndport, 2);

	ret = socks5_src_do_transmit(handle, session);

	return ret;
}

static int socks5_on_src_recv(struct sserver_handle *handle, struct ssession *session) {
	int ret = 0;
	
	switch(session->state) {
	case SSESSION_STATE_AUTH_METHOD:
		socks5_src_choose_auth_method(handle, session);
		break;	
	case SSESSION_STATE_AUTH_BEIGIN:
		socks5_src_do_auth(handle, session);
		break;
	case SSESSION_STATE_CONNECT:
		socks5_src_do_connect(handle, session);
		break;
	case SSESSION_STATE_TRANSMIT:
		FD_SET(session->srcfd, &handle->writefds);
		break;
	}
	return ret;
}

static int socks5_on_dst_recv(struct sserver_handle *handle, struct ssession *session) {
	int ret = 0;
	FD_SET(session->srcfd, &handle->writefds);
	return ret;
}

static int socks5_on_send(struct sserver_handle *handle, struct ssession *session, int iotype) {
	
	struct ringbuffer_tran tran;
    char buf[4096];
    int nread = 0;
    int nsent = 0;
    int nleft = 0;
    int count = 0;
    int fd = -1;
    struct ringbuffer *orgrb = NULL;
    struct ringbuffer *tranrb = NULL; 

    tranrb = ringbuffer_transaction_begin(session->srcbuf, &tran);
    
	switch(iotype) {
	case SSESSION_IO_TYPE_SRC:
		orgrb = session->srcbuf;
		fd = session->srcfd;
		break;
	case SSESSION_IO_TYPE_DST:
		orgrb = session->dstbuf;
		fd = session->dstfd;
		break;
	}

    while(nleft == 0) {
		if(ringbuffer_can_read(tranrb)) {

    		nread = ringbuffer_transaction_read(tranrb, buf, 4096);
    		if(nread < 0) {
       		 	ringbuffer_transaction_rollback(session->srcbuf, &tran);
        		return nread;
    		}

			nsent = send(fd, buf, nread);
			if(nsent <= 0) {
       		 	ringbuffer_transaction_rollback(session->srcbuf, &tran);
        		return nsent;
			}

			count += nsent;
			nleft = nread - nsent;
		} else {
			FD_CLR(fd, &handle->session->writefds);
			nleft = 0;
			break;
		}
	}
	
	ringbuffer_tran_set_left(&tran, nleft);
	ringbuffer_transaction_commit(session->srcbuf, &tran);

	return count;
}

static int socks5_on_recv(struct sserver_handle *handle, struct ssession *session, int iotype) {
	int ret = 0;
	switch(iotype) {
	case SSESSION_IO_TYPE_SRC:
		ret = socks5_on_src_recv(handle, session);
		break;
	case SSESSION_IO_TYPE_DST:
		ret = socks5_on_dst_recv(handle, session);
		break;
	}
	return ret;
}

struct sserver_handle *create_socks5_handle(struct sserver *server) {

	sserver_handle *handle = malloc(sizeof(*handle));
	if(!handle) {
		return NULL;
	}

	handle->server = server;
	handle->on_send = socks5_on_send;
	handle->on_recv = socks5_on_recv;
	return handle;
}