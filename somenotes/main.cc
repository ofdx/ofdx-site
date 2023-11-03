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
	class NoteDatabase {
		std::string m_dbpath;

		struct NoteFile {
			// Every NoteFile is represented by a file on disk, named m_id.
			// The file should have headers for each of the members of this
			// struct, plus the actual contents of the file (as m_body).
			uint64_t m_timeCreated, m_timeModified;
			std::string m_id, m_author, m_title, m_body;

			// When a file is modified, we can mark it as pendingSave, and
			// eventually it will be written to disk.
			bool m_pendingSave;

			NoteFile() :
				m_pendingSave(false)
			{}
		};

		std::unordered_map<std::string, std::shared_ptr<NoteFile>> m_notes;

		void get_the_rest(std::stringstream &src, std::string &dest) const {
			if(src >> dest){
				std::string buf;

				if(getline(src, buf))
					dest += buf;
			}
		}

	public:
		void load(){
			// Open m_dbpath and read every file.
			for(auto const& entry : std::filesystem::recursive_directory_iterator(m_dbpath)){
				if(entry.is_regular_file()){
					std::ifstream infile(entry.path());

					if(infile){
						std::shared_ptr<NoteFile> note = std::make_shared<NoteFile>();

						// The ID is simply the file name.
						note->m_id = entry.path().filename();

						// Read the headers
						{
							std::string line;
							while(getline(infile, line)){
								// End of headers.
								if(line.empty())
									break;

								std::stringstream hss(line);
								std::string param;

								if(hss >> param){
									if(param == "author")
										get_the_rest(hss, note->m_author);
									else if(param == "title")
										get_the_rest(hss, note->m_title);
									else if(param == "created")
										hss >> note->m_timeCreated;
									else if(param == "modified")
										hss >> note->m_timeModified;
								}
							}

							// Data repair as needed...
							if(note->m_timeModified < note->m_timeCreated)
								note->m_timeModified = note->m_timeCreated;
						}

						// Read body and store it.
						if(infile){
							std::stringstream body;
							std::string line;

							while(getline(infile, line))
								body << line << '\n';

							note->m_body = body.str();
						}

						// Store that bad boy in the map.
						m_notes[note->m_id] = note;
					}
				}
			}
		}

		void save(){
			// TODO - write out entire database to disk. Or write one file?
			// The persistence should probably be granual.
			// Modification time management?
		}

		// FIXME debug
		void debug(std::ostream &ss){
			for(auto &el : m_notes){
				ss << "[" << el.first << "] id[" << el.second->m_id << "] author[" << el.second->m_author << "] "
				<< "title[" << el.second->m_title << "] created[" << el.second->m_timeCreated << "] modified[" << el.second->m_timeModified << "] "
				<< "body[EOF]\n" << el.second->m_body << "EOF\n";
			}
		}

		NoteDatabase(std::string const& dbpath) :
			m_dbpath(dbpath)
		{
			load();
		}
	} *m_noteDb;

	void debugResponse(std::unique_ptr<dmitigr::fcgi::Server_connection> const& conn){
		if(!serveTemplatedDocument(conn, (m_cfg.m_templatePath + "debug/infopage.html"), true))
			serve404(conn);
	}

	void fillTemplate(std::unique_ptr<dmitigr::fcgi::Server_connection> const& conn, std::string const& text) override {
		std::stringstream tss(text);
		std::string word;

		if(tss >> word){
			if(word == "template"){
				// Load a file (which can itself also be templated)
				if(!(tss >> word))
					return;

				serveTemplatedDocument(conn, (m_cfg.m_templatePath + word));
			} else if(word == "auth"){
				// Related to authentication in some way.
				if(!(tss >> word))
					return;

				// Replace with the authenticated user's name
				if(word == "user")
					conn->out() << m_authUser;

			} else if(word == "debug"){
				// Debug stuff, probably going to be removed in the future.
				if(!(tss >> word))
					return;

				if(word == "table"){
					if(!(tss >> word))
						return;

					// Display a debug informational table.
					if(word == "parameters"){
						// Table of all parameters sent via FCGI
						conn->out() << "<table><tr><th>Name</th><th>Value</th></tr>";
						for(auto i = 0; i < conn->parameter_count(); ++ i){
							conn->out() << "<tr><td>" << conn->parameter_name(i) << "</td><td>" << conn->parameter(i) << "</td></tr>";
						}
						conn->out() << "</table>";
					} else if(word == "files"){
						// Dump the file database.
						m_noteDb->debug(conn->out());
					}
				} else if(word == "isauthd"){
					// Serve the next templated file if the user is authenticated.
					if(!m_authUser.empty()){
						std::string fname;

						if(tss >> fname)
							serveTemplatedDocument(conn, (m_cfg.m_templatePath + fname));
					}
				}
			}
		}
	}

	void serve404(std::unique_ptr<dmitigr::fcgi::Server_connection> const& conn){
		conn->out() << "Status: 404 Not Found\r\n";

		if(!serveTemplatedDocument(conn, (m_cfg.m_templatePath + "404.html")))
			// Misconfiguration or missing file...
			conn->out() << "\r\n\r\n404\r\n";
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
		m_noteDb(nullptr),
		m_cfg(PORT_OFDX_SOMENOTES, PATH_OFDX_SOMENOTES)
	{}

	bool processCliArguments(int argc, char **argv){
		if(m_cfg.processCliArguments(argc, argv)){
			// Database path must be set...
			if(m_cfg.m_dataPath.empty()){
				std::cerr << "Error: datapath must be set";
				return false;
			}
		}

		m_noteDb = new NoteDatabase(m_cfg.m_dataPath);

		return true;
	}

	void handleConnection(std::unique_ptr<dmitigr::fcgi::Server_connection> const& conn) override {
		std::string const SCRIPT_NAME(conn->parameter("SCRIPT_NAME"));
		parseCookies(conn);

		if(SCRIPT_NAME == PATH_OFDX_SOMENOTES){
			std::ifstream infile(m_cfg.m_templatePath + "index.html");

			if(!serveTemplatedDocument(conn, (m_cfg.m_templatePath + "index.html"), true))
				serve404(conn);
		} else if(SCRIPT_NAME == URL_NOTES_DEBUG){
			if(m_authUser.empty()){
				// Hide the debug page with the generic 404.
				serve404(conn);
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
	if(!app.processCliArguments(argc, argv))
		return 1;

	app.listen(app.m_cfg);

	while(app.accept());

	return 0;
}
