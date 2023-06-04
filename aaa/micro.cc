/*
   OFDX Microservice base class
   mperron (2023)
*/

#include "micro.h"

#include <fstream>

void OfdxManagerMicro::load(){
	std::ifstream infile(m_kvPath);

	if(infile){
		std::string line;

		while(getline(infile, line)){
			std::stringstream ss(line);
			std::string k, v;

			if(ss >> k >> v)
				m_kvStore[k] = v;
		}
	}
}

void OfdxManagerMicro::save(){
	std::ofstream outfile(m_kvPath);

	if(outfile){
		for(auto const& el : m_kvStore)
			outfile << el.first << " " << el.second << std::endl;

		m_modified = false;
	}
}


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

	if(m_modified)
		save();

	return true;
}

