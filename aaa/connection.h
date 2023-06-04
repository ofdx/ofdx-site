/*
	connection.h
	mperron(2022)
*/

#ifndef KWS3_CONNECTION_H
#define KWS3_CONNECTION_H

#include <sstream>
#include <unordered_map>

#include <arpa/inet.h>

class Connection {
	void connection_cleanup();

protected:
	bool m_valid;

	// The timeout is this value * 10ms. So default 500 = 5000ms = 5 seconds.
	int m_readAgainTimeout;
	int m_emptycounter;

	int m_fd;
	struct sockaddr_in m_addr_client;

	std::stringstream m_sockstream;

public:
	Connection() :
		m_valid(false),
		m_readAgainTimeout(500),
		m_emptycounter(0),
		m_fd(-1),
		m_addr_client({})
	{}

	virtual ~Connection(){
		connection_cleanup();
	}

	inline int fd(int val){ return(m_fd = val); }
	inline struct sockaddr_in& addr_client(){ return m_addr_client; }

	void prepareToRead();

	ssize_t tryRead();
	ssize_t tryWrite(const std::string &msg) const;

	virtual int readFailure(int code);
	virtual int readSuccess(ssize_t rc, const char *buf);

	virtual inline bool valid() const { return m_valid; }

};

#endif
