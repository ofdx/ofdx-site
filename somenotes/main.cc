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
public:
	OfdxSomeNotes() :
		OfdxFcgiService(9010, "/notes/")
	{
		// FIXME debug
		m_cfg.m_dataPath = "./";
	}

	void handleConnection(std::unique_ptr<dmitigr::fcgi::Server_connection> const& conn) override {
		conn->out()
			<< "Content-Type: text/html; charset=utf-8\r\n"
			<< "\r\n";

		conn->out()
			<< "<!DOCTYPE html><html><head><title>OFDX</title></head><body>" << std::endl
			<< "<p><i>Coming soon...</i></p>" << std::endl;

		// FIXME debug - Dump CGI parameters
		{
			conn->out() << "<table><tr><th>Name</th><th>Value</th></tr>";

			for(auto i = 0; i < conn->parameter_count(); ++ i){
				conn->out() << "<tr><td>" << conn->parameter_name(i) << "</td><td>" << conn->parameter(i) << "</td></tr>";
			}

			conn->out() << "</table>" << std::endl;
		}


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
