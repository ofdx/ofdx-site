/*
   AAA
   mperron (2023)

   Authentication system, with a web interface for login/logout via FastCGI.
*/

#include "fcgi/fcgi.hpp"
#include "base64.h"

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

	void sendBadRequest(std::unique_ptr<dmitigr::fcgi::Server_connection> const& conn){
		conn->out()
			<< "Status: 400 Bad Request\r\n"
			<< "Content-Type: text/plain; charset=utf-8\r\n"
			<< "\r\n"
			<< "Bad request." << std::endl;
	}

	void sendUnauthorized(std::unique_ptr<dmitigr::fcgi::Server_connection> const& conn){
		conn->out()
			<< "Status: 401 Unauthorized\r\n"
			<< "Content-Type: text/plain; charset=utf-8\r\n"
			<< "Set-Cookie: ofdx_auth=" << "" << "; Path=/; Max-Age=0\r\n"
			<< "\r\n"
			<< "Invalid user name or password." << std::endl;
	}

	void sendAuthorized(std::unique_ptr<dmitigr::fcgi::Server_connection> const& conn){
		conn->out()
			<< "Status: 204 No Content\r\n"
			<< "Set-Cookie: ofdx_auth=" << "FIXME" << "; Path=/\r\n"
			<< "\r\n";
	}

	void loginResponse(std::unique_ptr<dmitigr::fcgi::Server_connection> const& conn){
		try {
			if(conn->parameter("REQUEST_METHOD") == std::string("POST")){
				std::string_view http_auth = conn->parameter("HTTP_AUTHORIZATION");

				if((http_auth.find("Basic ") != 0) || (http_auth.size() < 7)){
					sendBadRequest(conn);
					return;
				}

				http_auth = http_auth.substr(6);

				for(auto const& c : http_auth){
					if(!(
						((c >= 'a') && (c <= 'z')) ||
						((c >= 'A') && (c <= 'Z')) ||
						((c >= '0') && (c <= '9')) ||
						(c == '+') || (c == '/') || (c == '=')
					)){
						sendBadRequest(conn);
						return;
					}
				}

				std::string http_auth_decoded = base64_decode(http_auth);

				std::string user, pass;
				{
					size_t n = http_auth_decoded.find(':');

					if((n > 0) && (http_auth_decoded.size() > (n + 2))){
						user = http_auth_decoded.substr(0, n);
						pass = http_auth_decoded.substr(n + 1);
					}
				}

				if(user.size()){
					for(auto & c : user){
						// To lowercase...
						if((c >= 'A') && (c <= 'Z'))
							c += 0x20;

						if(!(
							((c >= 'a') && (c <= 'z')) ||
							((c >= '0') && (c <= '9'))
						)){
							// Invalid user name
							sendUnauthorized(conn);
							return;
						}
					}

					std::string http_auth_reencoded = base64_encode(user + ":" + pass);

					// FIXME debug
					std::cerr << "user[" << user << "] pass[" << pass << "] base64[" << http_auth_reencoded << "]" << std::endl;

					sendAuthorized(conn);
					return;
				}

				sendUnauthorized(conn);
				return;
			} else {
				// Not a POST request
				sendBadRequest(conn);
				return;
			}
		} catch(...){
			// Likely caused by a missing Authorization header.
			sendBadRequest(conn);
			return;
		}
	}

	void handleConnection(std::unique_ptr<dmitigr::fcgi::Server_connection> const& conn) override {
		std::string const OFDX_USER("ofdx_user");
		std::string const OFDX_PASS("ofdx_pass");

		std::string const URL_LOGIN(m_cfg.m_baseUriPath + "login/");

		if(conn->parameter("SCRIPT_NAME") == URL_LOGIN){
			loginResponse(conn);
			return;
		}

		conn->out()
			<< "Content-Type: text/html; charset=utf-8\r\n"
			<< "\r\n";

		conn->out()
			<< "<!DOCTYPE html>" << std::endl
			<< "<html><head>" << std::endl
			<< "<title>OFDX AAA</title>" << std::endl
			<< "<script src=\"/ofdx/js/ofdx_async.js\"></script>" << std::endl
			<< "</head><body>" << std::endl
			<< "<p><i>Thanking you!</i></p>" << std::endl;

		// Display a login form
		conn->out()
			<< "<form id=ofdx_login method=POST action=" << URL_LOGIN << ">"
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

		conn->out() << "<script src=\"/ofdx/aaa/ofdx_auth.js\"></script>" << std::endl;

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
