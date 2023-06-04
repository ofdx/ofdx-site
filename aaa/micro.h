#include "ofdx/ofdx_fcgi.h"
#include "tcpListener.h"

class OfdxManagerMicro {
	class MicroConnection : public Connection {
		OfdxManagerMicro *m_pService;

	public:
		MicroConnection(OfdxManagerMicro *pService) :
			m_pService(pService)
		{}

		bool receiveCmd();
	};

protected:
	TcpListener m_listener;
	OfdxBaseConfig m_cfg;

	std::string m_kvPath;
	std::unordered_map<std::string, std::string> m_kvStore;

	bool m_modified;

	void load();
	void save();

	virtual void processCmd(std::string const& line, std::stringstream & response) {}

public:
	OfdxManagerMicro(int argc, char **argv, int port, std::string const& kvFile) :
		m_listener(port, true),
		m_cfg(port, ""),
		m_modified(false)
	{
		// datapath must be set on the command line.
		m_cfg.processCliArguments(argc, argv);
		m_kvPath = m_cfg.m_dataPath + kvFile;

		load();
	}

	bool run();

};

