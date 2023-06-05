/*
   OFDX Session Manager
   mperron (2023)

   A service which provides database access for user sessions.
   Session creation, verification, and deletion.

	// Create session - requires authentication key
	SESSION CREATE $name $key

	// Verify session
	SESSION VERIFY $id

	// Delete session
	SESSION DELETE $id

*/

#include "session_manager.h"

int main(int argc, char **argv){
	OfdxSessionManager server(argc, argv);

	while(server.run());
	return 0;
}


void OfdxSessionManager::processCmdSession(std::stringstream & cmd, std::stringstream & response){
	std::string op, arg;

	if(cmd >> op >> arg){
		if(op == "CREATE"){
			std::string key;

			if(cmd >> key){
				// Validate key against credentials file.
				if((m_credentialTable.count(arg) > 0) && (m_credentialTable[arg] == key)){
					std::string sid;

					// User is authenticated, we can generate a session ID.
					for(int attempts = 5; attempts > 0; -- attempts){
						createRandomSid(sid);

						if(m_sessionTable.count(sid) == 0){
							m_sessionTable[sid] = arg;
							m_modifiedSessionTable = true;

							response << sid;
							break;
						}
					}
				}
			}

			response << std::endl;
		} else if(op == "VERIFY"){
			// Returns the user name if the session exists.
			if(m_sessionTable.count(arg) != 0)
				response << m_sessionTable[arg];

			response << std::endl;
		} else if(op == "DELETE"){
			// Delete the specified session by ID, if it exists. Returns "OK".
			if(m_sessionTable.count(arg) != 0){
				m_sessionTable.erase(arg);
				m_modifiedSessionTable = true;
			}

			response << "OK" << std::endl;
		}
	}
}

void OfdxSessionManager::processCmdCredential(std::stringstream & cmd, std::stringstream & response){
	std::string op, arg;

	if(cmd >> op >> arg){
		// TODO ...
	}
}

void OfdxSessionManager::processCmd(std::string const& line, std::stringstream & response){
	std::stringstream cmd(line);
	std::string area, op, arg;

	if(cmd >> area){
		if(area == "SESSION")
			processCmdSession(cmd, response);
		else if(area == "CREDENTIAL")
			processCmdCredential(cmd, response);
	}

}

void OfdxSessionManager::loadSessions(){
	std::ifstream infile(m_cfg.m_dataPath + OFDX_FILE_SESS);

	if(infile){
		std::string line;

		while(getline(infile, line)){
			std::stringstream ss(line);
			std::string k, v;

			if(ss >> k >> v)
				m_sessionTable[k] = v;
		}
	}
}

void OfdxSessionManager::saveSessions(){
	std::ofstream outfile(m_cfg.m_dataPath + OFDX_FILE_SESS);

	if(outfile){
		for(auto const& el : m_sessionTable)
			outfile << el.first << " " << el.second << std::endl;

		m_modifiedSessionTable = false;
	}
}

void OfdxSessionManager::loadCredentials(){
	std::ifstream infile(m_cfg.m_dataPath + OFDX_FILE_CRED);

	if(infile){
		std::string line;

		while(getline(infile, line)){
			std::stringstream ss(line);
			std::string k, v;

			if(ss >> k >> v)
				m_credentialTable[k] = v;
		}
	}
}

void OfdxSessionManager::saveCredentials(){
	std::ofstream outfile(m_cfg.m_dataPath + OFDX_FILE_CRED);

	if(outfile){
		for(auto const& el : m_credentialTable)
			outfile << el.first << " " << el.second << std::endl;

		m_modifiedCredentialTable = false;
	}
}

void OfdxSessionManager::persist(){
	if(m_modifiedSessionTable)
		saveSessions();

	if(m_modifiedCredentialTable)
		saveCredentials();
}
