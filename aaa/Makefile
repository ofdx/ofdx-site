APP=ofdx_aaa
GPP=g++ -std=c++17 -I../ -I../dmitigr_fcgi/src/ -I../renényffenegger/

all: ${APP}

clean:
	rm -f ${APP}

run: all stop
	sleep 1
	( ./${APP} "datapath ~/test/db/aaa/" ) &

stop:
	( killall -q ${APP} ) &

${APP}: main.cc ../ofdx/* ../renényffenegger/rene.o
	${GPP} -o ${APP} main.cc ../renényffenegger/rene.o
