/*
   SomeNotes
   mperron (2024)

   A FastCGI web application for storing organized lists of links,
   random notes, small files, etc.
   
   What's more than "one note?"
   SomeNotes!
*/

#include "base64.h"
#include "ofdx/ofdx_fcgi.h"

#include <filesystem>

#define RESOURCE_VERSION 4

class OfdxSomeNotes : public OfdxFcgiService {
public:
	class NoteDatabase {
	public:
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

			void json(std::ostream &ss, bool withBody){
				ss << "{"
					<< "\"id\":\"" << m_id << "\","
					<< "\"title\":\"" << base64_encode(m_title) << "\","
					<< "\"created\":" << m_timeCreated << ","
					<< "\"modified\":" << m_timeModified;

				if(withBody)
					ss << ",\"body\":\"" << base64_encode(m_body) << "\"";

				ss << "}";
			}
		};

	private:
		std::string m_dbpath;
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

		void jsonMetadataForUser(std::ostream &ss, std::string const& user){
			bool first = true;

			ss << "{\"notes\":[";

			for(auto &el : m_notes){
				if(el.second->m_author != user)
					continue;

				if(first)
					first = false;
				else
					ss << ',';

				el.second->json(ss, false);
			}

			ss << "]}";
		}

		std::shared_ptr<NoteFile> getNote(std::string const& user, std::string const& fname){
			if(m_notes.count(fname) == 1){
				std::shared_ptr<NoteFile> note = m_notes[fname];

				if(note && (note->m_author == user))
					return note;
			}

			return nullptr;
		}

		NoteDatabase(std::string const& dbpath) :
			m_dbpath(dbpath)
		{
			load();
		}
	};

private:
	NoteDatabase *m_noteDb;

	void apiFileRoot(std::unique_ptr<dmitigr::fcgi::Server_connection> const& conn){
		std::string const REQUEST_METHOD(conn->parameter("REQUEST_METHOD"));
		/*
			f/
			  GET
				- Get metadata for all note files owned by the authorized user.
			  POST
				- Create a new note file. Returns the path to the new note file in the API,
				  or the file name, or some other identifier.
			  OPTIONS
		*/
		if(REQUEST_METHOD == "OPTIONS"){
			conn->out() << "Allow: OPTIONS, GET, POST\r\n\r\n";
		} else if(REQUEST_METHOD == "GET"){
			// FIXME debug - check m_authUser and return error
			conn->out()
				<< "Status: 200 OK\r\n"
				<< "Content-Type: application/json; charset=utf-8\r\n"
				<< "\r\n";

			// Get all files for this user
			m_noteDb->jsonMetadataForUser(conn->out(), m_authUser);
		} else if(REQUEST_METHOD == "POST"){
			// Create a file for this user
			// TODO
		}
	}
	void apiFile(std::unique_ptr<dmitigr::fcgi::Server_connection> const& conn, std::string const& fname){
		std::string const REQUEST_METHOD(conn->parameter("REQUEST_METHOD"));

		if(REQUEST_METHOD == "OPTIONS"){
			conn->out() << "Allow: OPTIONS, GET, PUT, DELETE\r\n\r\n";
		} else if(REQUEST_METHOD == "GET"){
			std::shared_ptr<NoteDatabase::NoteFile> note = m_noteDb->getNote(m_authUser, fname);

			if(note){
				// Note exists and this user can access it.
				conn->out()
					<< "Status: 200 OK\r\n"
					<< "Content-Type: application/json; charset=utf-8\r\n"
					<< "\r\n";

				note->json(conn->out(), true);
			} else {
				serve404(conn);
			}
		} else if(REQUEST_METHOD == "PUT"){
			// TODO - update this note
		} else if(REQUEST_METHOD == "DELETE"){
			// TODO - delete the specified note
		}
	}

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

			} else if(word == "isauthd"){
				// Serve the next templated file if the user is authenticated.
				if(!m_authUser.empty()){
					std::string fname;

					if(tss >> fname)
						serveTemplatedDocument(conn, (m_cfg.m_templatePath + fname));
				}

			} else if(word == "isnotauthd"){
				// Serve the next templated file if the user is NOT authenticated.
				if(m_authUser.empty()){
					std::string fname;

					if(tss >> fname)
						serveTemplatedDocument(conn, (m_cfg.m_templatePath + fname));
				}

			} else if(word == "auth"){
				// Related to authentication in some way.
				if(!(tss >> word))
					return;

				// Replace with the authenticated user's name
				if(word == "user")
					conn->out() << m_authUser;

			} else if(word == "version"){
				// Used for cache busting.
				conn->out() << RESOURCE_VERSION;

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
				}

			}
		}
	}

	void serve404(std::unique_ptr<dmitigr::fcgi::Server_connection> const& conn){
		conn->out()
			<< "Status: 404 Not Found\r\n"
			<< "Content-Type: text/html; charset=utf-8\r\n\r\n";

		if(!serveTemplatedDocument(conn, (m_cfg.m_templatePath + "404.html")))
			// Misconfiguration or missing file...
			conn->out() << "<html><head><title>404</title></head><body>404</body></html>\r\n";
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
		} else if(SCRIPT_NAME.find(URL_NOTES_FILE) == 0){
			// Note file API
			if(SCRIPT_NAME == URL_NOTES_FILE){
				// List all files or POST to create a new file.
				apiFileRoot(conn);
			} else {
				// The name of the file should be the rest of the SCRIPT_NAME after the API part.
				std::string const fname(SCRIPT_NAME.substr(URL_NOTES_FILE.size()));

				// Show or modify one specific file.
				apiFile(conn, fname);
			}
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
