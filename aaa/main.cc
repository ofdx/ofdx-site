/*
   AAA
   mperron (2023)

   Authentication system, with a web interface for login/logout via FastCGI.
*/

#include "base64.h"
#include "ofdx/ofdx_fcgi.h"

#include <unordered_map>


class OfdxAaa : public OfdxFcgiService {

	// Create a session ID for the specified user and return it in SID. Returns
	// false if the operation failed.
	bool getSid(std::string const& user, std::string const& addr, std::string const& key, std::string & sid) const {
		std::stringstream qss;

		qss << "SESSION CREATE " << user << " " << key << " " << addr;
		return querySessionDatabase(qss.str(), sid);
	}

	// Remove the session ID from the database (logout).
	void rmSid() const {
		try{
			std::string const session = m_cookies.at(OFDX_AUTH);
			std::string result;

			querySessionDatabase(std::string("SESSION DELETE ") + session, result);
		} catch(...){}
	}

public:
	struct OfdxBaseConfig m_cfg;

	OfdxAaa() :
		m_cfg(PORT_OFDX_AAA, PATH_OFDX_AAA)
	{}

	bool processCliArguments(int argc, char **argv){
		if(m_cfg.processCliArguments(argc, argv)){
			// Database path must be set...
			if(m_cfg.m_dataPath.empty()){
				std::cerr << "Error: datapath must be set";
				return false;
			}
		}

		return true;
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
			<< "Set-Cookie: " << OFDX_AUTH << "=" << "" << "; SameSite=Strict; Path=/; Max-Age=0\r\n"
			<< "\r\n"
			<< "Invalid user name or password." << std::endl;
	}

	void sendAuthorized(std::unique_ptr<dmitigr::fcgi::Server_connection> const& conn, std::string const& sid) const {
		conn->out()
			<< "Status: 204 No Content\r\n"
			<< "Set-Cookie: " << OFDX_AUTH << "=" << sid << "; SameSite=Strict; Path=/; Max-Age=" << (6 * 7 * 24 * 60 * 60) /* 6 weeks in seconds */ << "\r\n"
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
					{
						std::string sid, addr;

						// Fill in the client IP address if available.
						try {
							addr = conn->parameter("REMOTE_ADDR");
						} catch(...){}

						if(getSid(user, addr, http_auth_reencoded, sid)){
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
			<< "Set-Cookie: " << OFDX_AUTH << "=" << "" << "; SameSite=Strict; Path=/; Max-Age=0\r\n"
			<< "\r\n";
	}

	void handleConnection(std::unique_ptr<dmitigr::fcgi::Server_connection> const& conn) override {
		std::string const SCRIPT_NAME(conn->parameter("SCRIPT_NAME"));
		parseCookies(conn);

		if(SCRIPT_NAME == URL_LOGIN){
			loginResponse(conn);
		} else if(SCRIPT_NAME == URL_LOGOUT){
			rmSid();
			logoutResponse(conn);
		} else {
			// 404 for all other URIs
			conn->out()
				<< "Status: 404 Not Found\r\n"
				<< "Content-Type: text/plain; charset=utf-8\r\n"
				<< "\r\n"
				<< "You peer deeply into the darkness, yet detect nothing.";
		}
	}
};

int main(int argc, char **argv){
	OfdxAaa app;

	// Get config overrides from the CLI arguments.
	if(!app.processCliArguments(argc, argv))
		return 1;

	app.listen(app.m_cfg);

	while(app.accept());

	return 0;
}
