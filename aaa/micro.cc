/*
   OFDX Microservice base class
   mperron (2023)
*/

#include "micro.h"

bool OfdxManagerMicro::MicroConnection::receiveCmd(){
	prepareToRead();

	do {
		int rc = tryRead();

		if(rc < 0)
			// Retry
			continue;

		if(rc == 0)
			// 0 bytes read is generally a timeout.
			return false;

	} while(m_sockstream.str().find("\n\n") == std::string::npos);

	std::string line;

	while(getline(m_sockstream, line)){
		std::stringstream response;

		m_pService->processCmd(line, response);

		// Send the response if we have one.
		if(response.str().size())
			tryWrite(response.str());
	}

	// Close connection.
	return false;
}

bool OfdxManagerMicro::run(){
	MicroConnection conn(this);

	if(m_listener.accept(conn) >= 0)
		while(conn.receiveCmd());

	persist();

	return true;
}
