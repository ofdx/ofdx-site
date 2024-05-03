SOMENOTES=ofdx_somenotes
GPP=g++ -std=c++17 -I../ -I../dmitigr_fcgi/src/ -I../renényffenegger/

all: ${SOMENOTES}

clean:
	rm -f ${SOMENOTES}

run: all stop
	sleep 1
	( ./${SOMENOTES} \
		"templatepath /home/mike/src/ofdx/somenotes/notes/" \
		"datapath /home/mike/test/db/somenotes/" ) &

stop:
	( killall -q ${SOMENOTES} ) &

${SOMENOTES}: main.cc ../ofdx/* ../renényffenegger/rene.o
	${GPP} -o ${SOMENOTES} main.cc ../renényffenegger/rene.o
