APP=ofdx_aaa
GPP=g++ -std=c++17 -I../ -I../dmitigr_fcgi/src/

all: ${APP}

clean:
	rm -f ${APP}

run: all stop
	sleep 1
	( ./${APP} "datapath ~/test/db/aaa/" ) &

stop:
	( killall ${APP} > /dev/null 2>&1 ) &

${APP}: main.cc ../ofdx/*
	${GPP} -o ${APP} main.cc
