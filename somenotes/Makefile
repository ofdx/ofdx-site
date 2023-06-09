SOMENOTES=ofdx_somenotes
GPP=g++ -std=c++17 -I../ -I../dmitigr_fcgi/src/

all: ${SOMENOTES}

clean:
	rm -f ${SOMENOTES}

run: all stop
	sleep 1
	( ./${SOMENOTES} "templatepath /home/mike/src/ofdx/somenotes/templates/" ) &

stop:
	( killall -q ${SOMENOTES} ) &

${SOMENOTES}: main.cc ../ofdx/*
	${GPP} -o ${SOMENOTES} main.cc
