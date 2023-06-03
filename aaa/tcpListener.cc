/*
	tcpListener.cc
	mperron (2022)
*/

#include "tcpListener.h"

#include <unistd.h>

void TcpListener::cleanup(){
	if(!m_error){
		if(m_fd >= 0)
			close(m_fd);
	}
}

bool TcpListener::init(){
	if((m_fd = socket(AF_INET, (SOCK_STREAM | SOCK_CLOEXEC), 0)) == -1){
		m_error = 1;
		return false;
	}

	/*	SO_REUSEADDR is set so that we don't need to wait for all the
	 *	existing connections to close when restarting the server. */
	{
		int optval = 2;

		setsockopt(m_fd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
	}

	if(bind(m_fd, (struct sockaddr*) &m_addr, sizeof(m_addr)) == -1){
		m_error = 2;
		return false;
	}

	if(listen(m_fd, 64) == -1){
		m_error = 3;
		return false;
	}

	return true;
}

int TcpListener::accept(Connection &conn) const {
	struct sockaddr_in *addr_client = &(conn.addr_client());
	socklen_t client_length = sizeof(struct sockaddr_in);

	return conn.fd(accept4(m_fd, (struct sockaddr*) addr_client, &client_length, (SOCK_NONBLOCK | SOCK_CLOEXEC)));
}
