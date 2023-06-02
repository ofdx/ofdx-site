SOMENOTES=somenotes
GPP=g++ -std=c++17 -I../ -I../dmitigr_fcgi/src/

all: ${SOMENOTES}

clean:
	rm -f ${SOMENOTES}

run: all stop
	sleep 1
	( ./${SOMENOTES} ) &

stop:
	( killall ${SOMENOTES} > /dev/null 2>&1 ) &

${SOMENOTES}: main.cc ../ofdx/*
	${GPP} -o ${SOMENOTES} main.cc
