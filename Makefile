.PHONY:all clean

all: fuwuqi kehuduan

fuwuqi:fwq.cpp
	g++ -o $@ fwq.cpp

kehuduan:khd.cpp
	g++ -o $@ khd.cpp

clean:
	rm fuwuqi kehuduan
	