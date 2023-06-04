/*
   OFDX FCGI Common Code
   mperron (2023)
*/

struct OfdxBaseConfig {
	std::string m_addr;
	int m_port, m_backlog;

	std::string m_baseUriPath, m_dataPath;

	OfdxBaseConfig(int port, std::string const& baseUriPath) :
		m_addr("127.0.0.1"), m_port(port), m_backlog(64),

		m_baseUriPath(baseUriPath)
	{}

	virtual void receiveCliArgument(std::string const& k, std::string const& v) {}
	virtual void receiveCliOption(std::string const& opt) {}

	bool processCliArguments(int argc, char **argv){
		for(int i = 1; i < argc; ++ i){
			std::stringstream ss(argv[i]);
			std::string k, v;

			if(ss >> k){
				if(k == "port"){
					int vi;

					if(ss >> vi)
						m_port = vi;
				} else if(k == "backlog"){
					int vi;

					if(ss >> vi)
						m_backlog = vi;
				} else if(ss >> v){
					if(k == "addr"){
						m_addr.assign(v);
					} else if(k == "baseuri"){
						m_baseUriPath.assign(v);
					} else if(k == "datapath"){
						m_dataPath.assign(v);
					} else {
						receiveCliArgument(k, v);
					}
				} else {
					receiveCliOption(k);
				}
			}
		}

		return true;
	}
};

class OfdxFcgiService {
protected:
	OfdxBaseConfig m_cfg;
	std::shared_ptr<dmitigr::fcgi::Listener> m_pServer;

	virtual void handleConnection(std::unique_ptr<dmitigr::fcgi::Server_connection> const& conn) = 0;

	void parseCookies(std::unique_ptr<dmitigr::fcgi::Server_connection> const& conn, std::unordered_map<std::string, std::string> & cookies) const {
		try {
			std::string http_cookie(conn->parameter("HTTP_COOKIE"));
			std::stringstream http_cookie_ss(http_cookie);
			std::string cookie;

			while(http_cookie_ss >> cookie){
				size_t n = cookie.find('=');

				for(auto & c : cookie){
					if(c == ';'){
						c = 0;
						break;
					}
				}

				if((n > 0) && (n < cookie.size() - 1))
					cookies[cookie.substr(0, n)] = cookie.substr(n + 1);
			}

		} catch(...){}
	}

public:
	OfdxFcgiService(int const port, std::string const& baseUriPath) :
		m_cfg(port, baseUriPath)
	{}

	virtual bool processCliArguments(int argc, char **argv){
		return m_cfg.processCliArguments(argc, argv);
	}

	void listen(){
		if(!m_pServer){
			dmitigr::fcgi::Listener_options options {
				m_cfg.m_addr, m_cfg.m_port, m_cfg.m_backlog
			};

			m_pServer = std::make_shared<dmitigr::fcgi::Listener>(options);
			m_pServer->listen();
		}
	}

	bool accept(){
		try {
			if(const auto conn = m_pServer->accept())
				handleConnection(conn);

		} catch(std::exception const& e){
			std::cerr << "Error: " << e.what() << std::endl;
		}

		return true;
	}

	bool querySessionDatabase(std::string const& query, std::string & result) const {
		std::stringstream css;
		FILE *p = NULL;

		css << "cat << E=O=F | nc localhost 9001\n"
			<< query << "\n\n"
			<< "E=O=F";

		if(p = popen(css.str().c_str(), "r")){
			size_t n = 1024;
			char *raw = (char*) calloc(n, sizeof(char));

			getline(&raw, &n, p);
			if(char *s = strpbrk(raw, "\r\n"))
				*s = 0;

			result.assign(raw);

			free(raw);
			pclose(p);

			return !result.empty();
		}

		return false;
	}
};
