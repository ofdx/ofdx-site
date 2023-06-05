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

	virtual void processCmd(std::string const& line, std::stringstream & response) {}
	virtual void persist() {}

public:
	OfdxManagerMicro(int argc, char **argv, int port) :
		m_listener(port, true),
		m_cfg(port, "")
	{
		m_cfg.processCliArguments(argc, argv);
	}

	bool run();

};

