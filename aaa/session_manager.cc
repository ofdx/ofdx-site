/*
   OFDX Session Manager
   mperron (2023)

   A service which provides database access for user sessions.
   Session creation, verification, and deletion.
*/

#include <iostream>
#include <fstream>
#include <sstream>

#include <signal.h>
#include <unistd.h>

#include "tcpListener.h"

// Listen on port 9001/TCP.

/*
// Create session
SESSION CREATE $name

// Verify session
SESSION VERIFY $id

// Delete session
SESSION DELETE $id

*/

std::string const SESSION_DATABASE_FILENAME(std::string("/home/mike/test/db/aaa/") + "sess");

class OfdxSessionManager {

	TcpListener m_listener;

	// Key is session ID, value is user name.
	std::unordered_map<std::string, std::string> m_sessionTable;

	void load(){
		std::ifstream infile(SESSION_DATABASE_FILENAME);

		// Read all existing sessions.
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

	void save(){
		std::ofstream outfile(SESSION_DATABASE_FILENAME);

		// Write table to disk, replacing existing data.
		if(outfile){
			for(auto const& el : m_sessionTable){
				outfile << el.first << " " << el.second << std::endl;
			}
		}
	}

public:
	OfdxSessionManager() :
		m_listener(9001, true)
	{
		load();
	}

	bool run(){
		CmdConnection conn(&m_sessionTable);
		bool modified = false;

		if(m_listener.accept(conn) >= 0)
			while(conn.receiveCmd(modified));

		if(modified)
			save();

		return true;
	}
};

int main(int argc, char **argv){
	OfdxSessionManager server;

	while(server.run());
	return 0;
}
