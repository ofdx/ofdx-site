/*
	connection.cc
	mperron(2022)
*/

#include "connection.h"

#include <iostream>
#include <cstring>
#include <memory>
#include <fstream>

#include <unistd.h>
#include <sys/types.h>


using namespace std;

void Connection::connection_cleanup(){
	if(m_fd >= 0)
		close(m_fd);
}

void Connection::prepareToRead(){
	m_emptycounter = 0;
}

ssize_t Connection::tryRead(){
	static char buf[1024];
	memset(buf, 0, sizeof(buf));

	ssize_t rc = read(m_fd, &buf, sizeof(buf) - 1);
	return (rc < 0) ? readFailure(errno) : readSuccess(rc, buf);
}

ssize_t Connection::tryWrite(const string &msg) const {
	return write(m_fd, msg.c_str(), msg.size());
}

int Connection::readFailure(int code){
	if((code == EAGAIN) && (m_emptycounter ++ < m_readAgainTimeout)){
		usleep(1000);

		// Read will retry if return code is -1.
		return -1;
	}

	// Finished reading.
	return 0;
}

int Connection::readSuccess(ssize_t rc, const char *buf){
	if(rc > 0)
		m_sockstream << buf;

	return rc;
}
