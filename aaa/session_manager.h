/*
   OFDX Session Manager
   mperron (2023)
*/

#ifndef OFDX_SESSION_MANAGER_H
#define OFDX_SESSION_MANAGER_H

#include <iostream>
#include <fstream>
#include <sstream>

#include <ctime>

#include "base64.h"
#include "micro.h"

class OfdxSessionManager : public OfdxManagerMicro {
	struct OfdxSessionData {
		time_t m_startTime, m_expirationTime;
		std::string m_id, m_user, m_addr;

		OfdxSessionData(time_t const startTime, time_t const expirationTime, std::string const& id, std::string const& user, std::string const& addr) :
			m_startTime(startTime),
			m_expirationTime(expirationTime),
			m_id(id),
			m_user(user),
			m_addr(addr)
		{}

		OfdxSessionData() :
			OfdxSessionData(0, 0, "", "", "")
		{}

		OfdxSessionData(OfdxSessionData const& other){
			m_startTime = other.m_startTime;
			m_expirationTime = other.m_expirationTime;
			m_id = other.m_id;
			m_user = other.m_user;
			m_addr = other.m_addr;
		}

		bool parse(std::stringstream & ss){
			if(ss >> m_startTime >> m_expirationTime >> m_id >> m_user >> m_addr)
				return true;

			return false;
		}

		void write(std::ofstream & ss) const {
			ss
				<< m_startTime << " " << m_expirationTime << " "
				<< m_id << " " << m_user
				<< " " << (m_addr.empty() ? std::string("[]") : m_addr)
				<< std::endl;
		}
	};

	std::unordered_map<std::string, std::shared_ptr<OfdxSessionData>> m_sessionTable;
	std::unordered_map<std::string, std::string> m_credentialTable;

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
