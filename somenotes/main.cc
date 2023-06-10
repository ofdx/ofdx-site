/*
   SomeNotes
   mperron (2023)

   A FastCGI web application for storing organized lists of links,
   random notes, small files, etc.
   
   What's more than "one note?"
   SomeNotes!
*/

#include "ofdx/ofdx_fcgi.h"

#include <filesystem>


class OfdxSomeNotes : public OfdxFcgiService {
	void debugResponse(std::unique_ptr<dmitigr::fcgi::Server_connection> const& conn){
		std::ifstream infile(m_cfg.m_templatePath + "debug.html");

		if(infile)
			serveTemplatedDocument(conn, infile);
	}

	void loginResponse(std::unique_ptr<dmitigr::fcgi::Server_connection> const& conn){
		std::ifstream infile(m_cfg.m_templatePath + "login.html");

		if(infile && m_authUser.empty()){
			serveTemplatedDocument(conn, infile);
		} else {
			conn->out()
				<< "Status: 500 Error\r\n"
				<< "Content-Type: text/plain; charset=utf-8\r\n"
				<< "\r\n"
				<< "Something unexpected happened.";
		}
	}

	void fillTemplate(std::unique_ptr<dmitigr::fcgi::Server_connection> const& conn, std::string const& text) override {
		std::stringstream tss(text);
		std::string area;

		if(tss >> area){
			if(area == "auth"){
				// Related to authentication in some way.
				if(tss >> area){
					// Replace with the authenticated user's name
					if(area == "user")
						conn->out() << m_authUser;
				}
			} else if(area == "debug"){
				// Debug stuff, probably going to be removed in the future.
				if(tss >> area){
					if(area == "table"){
						// Display a debug informational table.
						if(tss >> area){
							if(area == "parameters"){
								// Table of all parameters sent via FCGI
								conn->out() << "<table><tr><th>Name</th><th>Value</th></tr>";
								for(auto i = 0; i < conn->parameter_count(); ++ i){
									conn->out() << "<tr><td>" << conn->parameter_name(i) << "</td><td>" << conn->parameter(i) << "</td></tr>";
								}
								conn->out() << "</table>";
							} else if(area == "files"){
								// Dump out all of the files in the working directory.
								conn->out() << "<table><tr><th>Filename</th><th>Size</th><th>Path</th></tr>";

								for(auto const& entry : std::filesystem::recursive_directory_iterator(m_cfg.m_dataPath)){
									conn->out()
										<< "<tr><td>" << entry.path().filename()
										<< "</td><td>" << (entry.is_directory() ? std::string("<i>dir</i>") : std::to_string(entry.file_size()))
										<< "</td><td>" << entry.path()
										<< "</td></tr>";
								}

								conn->out() << "</table>";
							}
						}
					} else if(area == "isauthd"){
						// Serve the next templated file if the user is authenticated.
						if(!m_authUser.empty()){
							std::string fname;

							if(tss >> fname){
								std::ifstream infile(m_cfg.m_templatePath + fname);

								if(infile)
									serveTemplatedDocument(conn, infile);
							}
						}
					}
				}
			}
		}
	}

	void serve404(std::unique_ptr<dmitigr::fcgi::Server_connection> const& conn){
		std::ifstream infile(m_cfg.m_templatePath + "404.html");

		if(infile){
			serveTemplatedDocument(conn, infile);
		} else {
			// Misconfiguration or missing file...
			conn->out() << "Status: 404\r\n\r\n";
		}
	}

public:
	struct Config : public OfdxBaseConfig {
		std::string m_templatePath;

		Config(int port, std::string const& baseUriPath) :
			OfdxBaseConfig(port, baseUriPath)
		{}

		void receiveCliArgument(std::string const& k, std::string const& v) override {
			if(k == "templatepath")
				m_templatePath.assign(v);
		}
	} m_cfg;

	OfdxSomeNotes() :
		m_cfg(PORT_OFDX_SOMENOTES, PATH_OFDX_SOMENOTES)
	{
		// FIXME debug
		m_cfg.m_dataPath = "./";
	}

	void handleConnection(std::unique_ptr<dmitigr::fcgi::Server_connection> const& conn) override {
		std::string const SCRIPT_NAME(conn->parameter("SCRIPT_NAME"));
		parseCookies(conn);

		if(SCRIPT_NAME == PATH_OFDX_SOMENOTES){
			std::ifstream infile(m_cfg.m_templatePath + "home.html");

			if(infile){
				conn->out()
					<< "Content-Type: text/html; charset=utf-8\r\n"
					<< "\r\n";

				serveTemplatedDocument(conn, infile);
			} else {
				serve404(conn);
			}
		} else if(SCRIPT_NAME == URL_NOTES_DEBUG){
			if(m_authUser.empty()){
				// Show login page...
				loginResponse(conn);
			} else {
				// Debug informational page.
				debugResponse(conn);
			}
		} else {
			serve404(conn);
		}
	}
};


int main(int argc, char **argv){
	OfdxSomeNotes app;

	// Get config overrides from the CLI arguments.
	if(!app.m_cfg.processCliArguments(argc, argv))
		return 1;

	app.listen(app.m_cfg);

	while(app.accept());

	return 0;
}
