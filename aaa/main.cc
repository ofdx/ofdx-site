/*
   AAA
   mperron (2023)

   Authentication system, with a web interface for login/logout via FastCGI.
*/

#include "fcgi/fcgi.hpp"

#include <iostream>
#include <sstream>
#include <filesystem>

#include "ofdx/ofdx_fcgi.h"

class OfdxAaa : public OfdxFcgiService {
public:
	OfdxAaa() :
		OfdxFcgiService(9000, "/aaa/")
	{}

	bool processCliArguments(int argc, char **argv) override {
		if(OfdxFcgiService::processCliArguments(argc, argv)){
			// Database path must be set...
			if(m_cfg.m_dataPath.empty()){
				std::cerr << "Error: datapath must be set";
				return false;
			}
		}

		return true;
	}

	void handleConnection(std::unique_ptr<dmitigr::fcgi::Server_connection> const& conn) override {
		std::string const OFDX_USER("ofdx_user");
		std::string const OFDX_PASS("ofdx_pass");

		conn->out()
			<< "Content-Type: text/html; charset=utf-8\r\n"
			<< "\r\n";

		conn->out()
			<< "<!DOCTYPE html><html><head><title>OFDX AAA</title></head><body>" << std::endl
			<< "<p><i>Thanking you!</i></p>" << std::endl;

		// Display a login form
		conn->out()
			<< "<form method=POST action=" << m_cfg.m_baseUriPath << "login>"
			<< "<label for=" << OFDX_USER << ">Username: </label><input id=" << OFDX_USER << " name=" << OFDX_USER << "><br>"
			<< "<label for=" << OFDX_PASS << ">Password: </label><input id=" << OFDX_PASS << " name=" << OFDX_PASS << " type=password><br>"
			<< "<input type=submit value=\"Login\"><br>"
			<< "</form>" << std::endl;

		// Display request body
		try {
			std::string content_type(conn->parameter("CONTENT_TYPE"));
			int content_length(std::stoi(std::string(conn->parameter("CONTENT_LENGTH"))));

			// FIXME debug
			conn->out()
				<< "<p>Received POST data of type \"<b>" << content_type << "</b>\" and "
				<< "length <b>" << content_length << "</b></p>" << std::endl;

			if(content_length > 0){
				std::string line;

				getline(conn->in(), line);

				// FIXME debug
				conn->out() << "<pre>" << line << "</pre>" << std::endl;
			}

		} catch(...){}

		conn->out() << "</body></html>";
	}
};

int main(int argc, char **argv){
	OfdxAaa app;

	// Get config overrides from the CLI arguments.
	if(!app.processCliArguments(argc, argv))
		return 1;

	app.listen();

	while(app.accept());

	return 0;
}
