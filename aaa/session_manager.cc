/*
   OFDX Session Manager
   mperron (2023)

   A service which provides database access for user sessions.
   Session creation, verification, and deletion.
*/

#include <iostream>

#include <signal.h>
#include <unistd.h>

#include "tcpListener.h"

// Listen on port 9001/TCP.

// Protocol is text based.
// The first line describes the action
// The following lines are a keyword, whitespace, and then a value.
// An empty line indicates the end of the request.

/*
// Create session
SESSION CREATE
user $name

// Verify session
SESSION VERIFY
id $sessionid

// Delete session
SESSION DELETE
id $sessionid

*/

class OfdxSessionManager {
	TcpListener m_listener;

public:
	OfdxSessionManager() :
		m_listener(9001, true)
	{}

	bool run(){
		Connection conn;

		if(m_listener.accept(conn) >= 0)
			while(conn.receiveCmd());

		return true;
	}
};

int main(int argc, char **argv){
	OfdxSessionManager server;

	while(server.run());
	return 0;
}
