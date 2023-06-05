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
	std::unordered_map<std::string, std::string> m_sessionTable, m_credentialTable;

	bool m_modifiedSessionTable;
	bool m_modifiedCredentialTable;


	// Creates a unique session ID.
	void createRandomSid(std::string & data){
		size_t const sizeOfSid(128);

		std::shared_ptr<unsigned char[]> raw(new unsigned char[sizeOfSid]);
		std::ifstream infile("/dev/urandom");

		if(raw && infile && infile.read((char*) raw.get(), sizeOfSid))
			data = base64_encode(raw.get(), sizeOfSid, true);
	}

	// CLI Command handling
	void processCmd(std::string const& line, std::stringstream & response) override;
	void processCmdSession(std::stringstream & cmd, std::stringstream & response);
	void processCmdCredential(std::stringstream & cmd, std::stringstream & response);

	// Disk operations
	void persist() override;
	void loadSessions();
	void saveSessions();
	void loadCredentials();
	void saveCredentials();

public:
	OfdxSessionManager(int argc, char **argv) :
		OfdxManagerMicro(argc, argv, PORT_OFDX_AAA_SESSION_MGR),
		m_modifiedSessionTable(false),
		m_modifiedCredentialTable(false)
	{
		// datapath must be set in the CLI arguments...
		loadSessions();
		loadCredentials();
	}
};

#endif
