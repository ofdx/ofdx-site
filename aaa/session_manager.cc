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

#include "ofdx/ofdx_fcgi.h"

/*
// Create session
SESSION CREATE $name

// Verify session
SESSION VERIFY $id

// Delete session
SESSION DELETE $id

*/

class OfdxSessionManager {
	TcpListener m_listener;
	OfdxBaseConfig m_cfg;

	// Key is session ID, value is user name.
	std::unordered_map<std::string, std::string> m_sessionTable;

	// Read existing sessions from the file on disk.
	void load(){
		std::ifstream infile(m_cfg.m_dataPath + "sess");

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

	// Update file on disk to match our current session cache structure.
	void save(){
		std::ofstream outfile(m_cfg.m_dataPath + "sess");

		// Write table to disk, replacing existing data.
		if(outfile){
			for(auto const& el : m_sessionTable){
				outfile << el.first << " " << el.second << std::endl;
			}
		}
	}

public:
	OfdxSessionManager(int argc, char **argv) :
		m_listener(PORT_OFDX_AAA_SESSION_MGR, true),
		m_cfg(PORT_OFDX_AAA_SESSION_MGR, "")
	{
		// datapath must be set on the command line.
		m_cfg.processCliArguments(argc, argv);
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
	OfdxSessionManager server(argc, argv);

	while(server.run());
	return 0;
}
