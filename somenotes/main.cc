/*
   SomeNotes
   mperron (2023)

   A FastCGI web application for storing organized lists of links,
   random notes, small files, etc.
   
   What's more than "one note?"
   SomeNotes!
*/

#include "ofdx/ofdx_fcgi.h"

#include <iostream>
#include <sstream>
#include <filesystem>


class OfdxSomeNotes : public OfdxFcgiService {
	void debugResponse(std::unique_ptr<dmitigr::fcgi::Server_connection> const& conn){
		conn->out()
			<< "Content-Type: text/html; charset=utf-8\r\n"
			<< "\r\n"
			<< "<!DOCTYPE html><html><head><title>OFDX - Debug</title></head><body>" << std::endl;

		if(!m_authUser.empty()){
			// Greeting and logout link.
			conn->out()
				<< "<p>Welcome <b>" << m_authUser << "</b>!</p>" << std::endl
				<< "<p><a href=\"" << URL_LOGOUT << "\">Logout</a>.</p>" << std::endl;
		}

		// Table of all parameters sent via FCGI
		conn->out() << "<table><tr><th>Name</th><th>Value</th></tr>";
		for(auto i = 0; i < conn->parameter_count(); ++ i){
			conn->out() << "<tr><td>" << conn->parameter_name(i) << "</td><td>" << conn->parameter(i) << "</td></tr>";
		}
		conn->out() << "</table>" << std::endl;

		// FIXME debug - Dump data path files
		{
			conn->out() << "<table><tr><th>Filename</th><th>Size</th><th>Path</th></tr>";

			for(auto const& entry : std::filesystem::recursive_directory_iterator(m_cfg.m_dataPath)){
				conn->out()
					<< "<tr><td>" << entry.path().filename()
					<< "</td><td>" << (entry.is_directory() ? std::string("<i>dir</i>") : std::to_string(entry.file_size()))
					<< "</td><td>" << entry.path()
					<< "</td></tr>";
			}

			conn->out() << "</table>" << std::endl;
		}

		conn->out() << "</body></html>";
	}

	void loginResponse(std::unique_ptr<dmitigr::fcgi::Server_connection> const& conn){
		if(m_authUser.empty()){
			// Show login page.
			conn->out()
				<< "Content-Type: text/html; charset=utf-8\r\n"
				<< "\r\n"
				<< "<!DOCTYPE html><html><head><title>OFDX - Login</title></head><body>" << std::endl
				<< "<script src=\"/ofdx/js/ofdx_async.js\"></script>" << std::endl
				<< "<script src=\"/ofdx/aaa/ofdx_auth.js\"></script>" << std::endl

				<< "<form id=ofdx_login method=POST action=" << URL_LOGIN << ">"
				<< "<label for=" << OFDX_USER << ">Username: </label><input id=" << OFDX_USER << " name=" << OFDX_USER << "><br>"
				<< "<label for=" << OFDX_PASS << ">Password: </label><input id=" << OFDX_PASS << " name=" << OFDX_PASS << " type=password><br>"
				<< "<input type=submit value=\"Login\"><br>"
				<< "</form>" << std::endl

				<< "</body></html>" << std::endl;
		} else {
			conn->out()
				<< "Status: 500 Error\r\n"
				<< "Content-Type: text/plain; charset=utf-8\r\n"
				<< "\r\n"
				<< "Something unexpected happened.";
		}
	}



public:
	OfdxSomeNotes() :
		OfdxFcgiService(PORT_OFDX_SOMENOTES, PATH_OFDX_SOMENOTES)
	{
		// FIXME debug
		m_cfg.m_dataPath = "./";
	}

	void handleConnection(std::unique_ptr<dmitigr::fcgi::Server_connection> const& conn) override {
		std::string const SCRIPT_NAME(conn->parameter("SCRIPT_NAME"));
		parseCookies(conn);

		if(SCRIPT_NAME == URL_NOTES_DEBUG){
			if(m_authUser.empty()){
				// Show login page...
				loginResponse(conn);
			} else {
				// Debug informational page.
				debugResponse(conn);
			}
		} else {
			// 404 for all other URIs
			conn->out()
				<< "Status: 404 Not Found\r\n"
				<< "Content-Type: text/plain; charset=utf-8\r\n"
				<< "\r\n"
				<< "No notes here.";
		}
	}
};


int main(int argc, char **argv){
	OfdxSomeNotes app;

	// Get config overrides from the CLI arguments.
	if(!app.processCliArguments(argc, argv))
		return 1;

	app.listen();

	while(app.accept());

	return 0;
}
