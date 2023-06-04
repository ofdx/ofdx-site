/*
   AAA
   mperron (2023)

   Authentication system, with a web interface for login/logout via FastCGI.
*/

#include "fcgi/fcgi.hpp"
#include "base64.h"

#include <iostream>
#include <sstream>
#include <fstream>
#include <unordered_map>

#include "ofdx/ofdx_fcgi.h"

std::string const OFDX_AUTH("ofdx_auth");

class OfdxAaa : public OfdxFcgiService {

	// Create a session ID for the specified user and return it in SID. Returns
	// false if the operation failed.
	bool getSid(std::string const& user, std::string & sid) const {
		return querySessionDatabase(std::string("CREATE ") + user, sid);
	}

	// Return the username associated with this active session ID.
	bool getUser(std::string const& sid, std::string & user) const {
		return querySessionDatabase(std::string("VERIFY ") + sid, user);
	}

	// Remove the session ID from the database (logout).
	void rmSid(std::string const& sid) const {
		std::string result;

		querySessionDatabase(std::string("DELETE ") + sid, result);
	}

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

	// Check whether the provided user and authorization key combination is valid.
	bool checkCredentials(std::string const& user, std::string const& auth) const {
		std::ifstream infile(m_cfg.m_dataPath + "cred");

		if(infile){
			std::string line;

			while(getline(infile, line)){
				std::stringstream ss(line);
				std::string k, v;

				if(ss >> k >> v){
					// Line has two words on it, could be "name key"
					if((k == user) && (v == auth))
						return true;
				}
			}
		}

		return false;
	}

	void sendBadRequest(std::unique_ptr<dmitigr::fcgi::Server_connection> const& conn) const {
		conn->out()
			<< "Status: 400 Bad Request\r\n"
			<< "Content-Type: text/plain; charset=utf-8\r\n"
			<< "\r\n"
			<< "Bad request." << std::endl;
	}

	void sendUnauthorized(std::unique_ptr<dmitigr::fcgi::Server_connection> const& conn) const {
		conn->out()
			<< "Status: 401 Unauthorized\r\n"
			<< "Content-Type: text/plain; charset=utf-8\r\n"
			<< "Set-Cookie: " << OFDX_AUTH << "=" << "" << "; Path=/; Max-Age=0\r\n"
			<< "\r\n"
			<< "Invalid user name or password." << std::endl;
	}

	void sendAuthorized(std::unique_ptr<dmitigr::fcgi::Server_connection> const& conn, std::string const& sid) const {
		conn->out()
			<< "Status: 204 No Content\r\n"
			<< "Set-Cookie: " << OFDX_AUTH << "=" << sid << "; Path=/\r\n"
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

					// If the credentials are OK, report success
					if(checkCredentials(user, http_auth_reencoded)){
						std::string sid;

						if(getSid(user, sid)){
							sendAuthorized(conn, sid);
							return;
						}
					}
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

	void logoutResponse(std::unique_ptr<dmitigr::fcgi::Server_connection> const& conn) const {
		std::string referer;

		try {
			referer = conn->parameter("HTTP_REFERER");
		} catch(...){
			referer = "/";
		}

		conn->out()
			<< "Status: 302 Found\r\n"
			<< "Location: " << referer << "\r\n"
			<< "Set-Cookie: " << OFDX_AUTH << "=" << "" << "; Path=/; Max-Age=0\r\n"
			<< "\r\n";
	}

	void handleConnection(std::unique_ptr<dmitigr::fcgi::Server_connection> const& conn) override {
		std::string const OFDX_USER("ofdx_user");
		std::string const OFDX_PASS("ofdx_pass");
		std::string const OFDX_REDIR("ofdx_redir");

		std::string const URL_LOGIN(m_cfg.m_baseUriPath + "login/");
		std::string const URL_LOGOUT(m_cfg.m_baseUriPath + "logout/");

		std::string const SCRIPT_NAME(conn->parameter("SCRIPT_NAME"));

		std::string user, session;
		{
			// Check cookies for session ID.
			std::unordered_map<std::string, std::string> cookies;
			parseCookies(conn, cookies);

			if(cookies.count(OFDX_AUTH)){
				session = cookies[OFDX_AUTH];
				getUser(session, user);
			}
		}

		if(SCRIPT_NAME == URL_LOGIN){
			loginResponse(conn);
			return;
		} else if(SCRIPT_NAME == URL_LOGOUT){
			rmSid(session);
			logoutResponse(conn);
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

		if(user.empty()){
			// Display a login form
			conn->out()
				<< "<form id=ofdx_login method=POST action=" << URL_LOGIN << ">"
				<< "<label for=" << OFDX_USER << ">Username: </label><input id=" << OFDX_USER << " name=" << OFDX_USER << "><br>"
				<< "<label for=" << OFDX_PASS << ">Password: </label><input id=" << OFDX_PASS << " name=" << OFDX_PASS << " type=password><br>"
				//<< "<input type=hidden name=" << OFDX_REDIR << " value=/notes/>"
				<< "<input type=submit value=\"Login\"><br>"
				<< "</form>" << std::endl;
		} else {
			conn->out()
				<< "<p>Welcome <b>" << user << "</b>!</p>" << std::endl
				<< "<p><a href=\"" << URL_LOGOUT << "\">Click here</a> to logout.</p>" << std:: endl;
		}

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
