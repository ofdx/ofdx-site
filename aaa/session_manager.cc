/*
   OFDX Session Manager
   mperron (2023)

   A service which provides database access for user sessions.
   Session creation, verification, and deletion.

	// Create session
	SESSION CREATE $name

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

void OfdxSessionManager::processCmd(std::string const& line, std::stringstream & response){
	std::stringstream cmd(line);
	std::string op, arg;

	if(cmd >> op >> arg){
		if(op == "CREATE"){
			// Create a new session and return the ID.
			std::string sid;

			for(int attempts = 5; attempts > 0; -- attempts){
				createRandomSid(sid);

				if(m_kvStore.count(sid) == 0){
					m_kvStore[sid] = arg;
					m_modified = true;

					response << sid;
					break;
				}
			}

			response << std::endl;
		} else if(op == "VERIFY"){
			// Returns the user name if the session exists.
			if(m_kvStore.count(arg) != 0)
				response << m_kvStore[arg];

			response << std::endl;
		} else if(op == "DELETE"){
			// Delete the specified session by ID, if it exists. Returns "OK".
			if(m_kvStore.count(arg) != 0){
				m_kvStore.erase(arg);
				m_modified = true;
			}

			response << "OK" << std::endl;
		}
	}
}
