APP=ofdx_aaa
SESSION_MGR=ofdx_sessionmgr
CREDENTIAL_MGR=ofdx_credentialmgr

GPP=g++ -std=c++17 -I../ -I../dmitigr_fcgi/src/ -I../renényffenegger/

all: ${APP} ${SESSION_MGR}

clean:
	rm -f ${APP} ${SESSION_MGR}

run: all stop
	( ./${APP} "datapath /home/mike/test/db/aaa/" ) &
	( ./${SESSION_MGR} "datapath /home/mike/test/db/aaa/" ) &

stop:
	killall -q ${APP} || true
	killall -q ${SESSION_MGR} || true

# Authentication FCGI Service
${APP}: main.cc ../ofdx/* ../renényffenegger/rene.o
	${GPP} -o ${APP} main.cc ../renényffenegger/rene.o

# Session manager service
${SESSION_MGR}: session_manager.* connection.* tcpListener.* micro.* ../ofdx/* ../renényffenegger/rene.o
	${GPP} -o ${SESSION_MGR} session_manager.cc connection.cc tcpListener.cc micro.cc ../renényffenegger/rene.o
