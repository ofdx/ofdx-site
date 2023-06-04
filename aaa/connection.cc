/*
	connection.cc
	mperron(2022)
*/

#include "connection.h"
#include "base64.h"

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


void createRandomSid(std::string & data){
	size_t const sizeOfSid(256);

	std::shared_ptr<unsigned char[]> raw(new unsigned char[sizeOfSid]);
	std::ifstream infile("/dev/urandom");

	if(raw && infile && infile.read((char*) raw.get(), sizeOfSid))
		data = base64_encode(raw.get(), sizeOfSid, true);
}

bool CmdConnection::receiveCmd(bool & modified){
	prepareToRead();

	do {
		int rc = tryRead();

		if(rc < 0)
			// Retry
			continue;

		if(rc == 0)
			// 0 bytes read is generally a timeout.
			return false;

	} while(m_sockstream.str().find("\n\n") == string::npos);

	// Parse commands
	std::string line;
	while(getline(m_sockstream, line)){
		std::stringstream cmd(line), oss;
		std::string op, arg;

		if(cmd >> op >> arg){
			if(op == "CREATE"){
				// Create a new session and return the ID.
				std::string sid;

				for(int attempts = 5; attempts > 0; -- attempts){
					createRandomSid(sid);

					if(m_pSessionTable->count(sid) == 0){
						(*m_pSessionTable)[sid] = arg;
						modified = true;

						oss << sid;
						break;
					}
				}

				oss << std::endl;
			} else if(op == "VERIFY"){
				// Returns the user name if the session exists.
				if(m_pSessionTable->count(arg) != 0)
					oss << (*m_pSessionTable)[arg];

				oss << std::endl;
			} else if(op == "DELETE"){
				// Delete the specified session by ID, if it exists. Returns "OK".
				if(m_pSessionTable->count(arg) != 0){
					m_pSessionTable->erase(arg);
					modified = true;
				}

				oss << "OK" << std::endl;
			}

			// Send the response
			if(oss.str().size())
				tryWrite(oss.str());
		}
	}

	// Close connection.
	return false;
}
