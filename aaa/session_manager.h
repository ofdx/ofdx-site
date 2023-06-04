/*
   OFDX Session Manager
   mperron (2023)
*/

#ifndef OFDX_SESSION_MANAGER_H
#define OFDX_SESSION_MANAGER_H

#include <iostream>
#include <fstream>
#include <sstream>

#include "base64.h"
#include "micro.h"

class OfdxSessionManager : public OfdxManagerMicro {
	void createRandomSid(std::string & data){
		size_t const sizeOfSid(256);

		std::shared_ptr<unsigned char[]> raw(new unsigned char[sizeOfSid]);
		std::ifstream infile("/dev/urandom");

		if(raw && infile && infile.read((char*) raw.get(), sizeOfSid))
			data = base64_encode(raw.get(), sizeOfSid, true);
	}

	void processCmd(std::string const& line, std::stringstream & response) override;

public:
	OfdxSessionManager(int argc, char **argv) :
		OfdxManagerMicro(argc, argv, PORT_OFDX_AAA_SESSION_MGR, OFDX_FILE_SESS)
	{
		// ...
	}
};

#endif
