APP=ofdx_aaa
SESSION_MGR=ofdx_sessionmgr

GPP=g++ -std=c++17 -I../ -I../dmitigr_fcgi/src/ -I../renényffenegger/

all: ${APP} ${SESSION_MGR}

clean:
	rm -f ${APP} ${SESSION_MGR}

run: all stop
	sleep 1
	( ./${APP} "datapath /home/mike/test/db/aaa/" ) &
	( ./${SESSION_MGR} ) &

stop:
	( killall -q ${APP} ) &
	( killall -q ${SESSION_MGR} ) &

# Authentication FCGI Service
${APP}: main.cc ../ofdx/* ../renényffenegger/rene.o
	${GPP} -o ${APP} main.cc ../renényffenegger/rene.o


# Session manager service
${SESSION_MGR}: session_manager.cc connection.* tcpListener.*
	${GPP} -o ${SESSION_MGR} session_manager.cc connection.cc tcpListener.cc
