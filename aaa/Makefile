APP=ofdx_aaa
GPP=g++ -std=c++17 -I../ -I../dmitigr_fcgi/src/

all: ${APP}

clean:
	rm -f ${APP}

run: all stop
	sleep 1
	( ./${APP} "datapath ~/test/db/aaa/" ) &

stop:
	( killall -q ${APP} ) &

${APP}: main.cc ../ofdx/*
	${GPP} -o ${APP} main.cc
